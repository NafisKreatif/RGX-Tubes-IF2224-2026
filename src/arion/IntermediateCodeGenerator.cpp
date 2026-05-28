#include "IntermediateCodeGenerator.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

using namespace arion;

namespace {
    std::string lowerCopy(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    bool isStatementContainer(ASTNodeKind kind) {
        return kind == ASTNodeKind::Program ||
               kind == ASTNodeKind::Block ||
               kind == ASTNodeKind::CompoundStatement ||
               kind == ASTNodeKind::StatementList;
    }
}

IntermediateCodeGeneratorError::IntermediateCodeGeneratorError(const std::string &message)
    : std::runtime_error(message) {}

IntermediateCodeGenerator::IntermediateCodeGenerator(const SymbolTable &symbolTable)
    : symbolTable_(symbolTable) {}

IntermediateCode IntermediateCodeGenerator::generate(const ASTNode &decoratedAst) {
    code_ = IntermediateCode();
    generateProgram(decoratedAst);
    return code_;
}

void IntermediateCodeGenerator::generateProgram(const ASTNode &node) {
    if (node.getKind() != ASTNodeKind::Program) {
        throw IntermediateCodeGeneratorError("Intermediate code generation must start from Program node");
    }

    code_.emitINT(frameSize(node), "init stack frame");
    generateNode(node);
    code_.emitRET("return");
}

void IntermediateCodeGenerator::generateNode(const ASTNode &node) {
    switch (node.getKind()) {
        case ASTNodeKind::Program:
        case ASTNodeKind::Block:
        case ASTNodeKind::CompoundStatement:
        case ASTNodeKind::StatementList:
            for (const ASTChild &child : node.getChildren()) {
                if (child.role == ASTChildRole::Declaration) {
                    continue;
                }
                generateNode(child.node);
            }
            break;

        case ASTNodeKind::Declarations:
        case ASTNodeKind::ConstDeclarations:
        case ASTNodeKind::TypeDeclarations:
        case ASTNodeKind::VarDeclarations:
        case ASTNodeKind::ConstDeclaration:
        case ASTNodeKind::TypeDeclaration:
        case ASTNodeKind::VarDeclaration:
        case ASTNodeKind::FieldDeclaration:
        case ASTNodeKind::EmptyStatement:
            break;

        default:
            generateStatement(node);
            break;
    }
}

void IntermediateCodeGenerator::generateStatement(const ASTNode &node) {
    switch (node.getKind()) {
        case ASTNodeKind::Assignment:
            generateAssignment(node);
            break;
        case ASTNodeKind::IfStatement:
            generateIfStatement(node);
            break;
        case ASTNodeKind::CaseStatement:
            generateCaseStatement(node);
            break;
        case ASTNodeKind::WhileStatement:
            generateWhileStatement(node);
            break;
        case ASTNodeKind::RepeatStatement:
            generateRepeatStatement(node);
            break;
        case ASTNodeKind::ForStatement:
            generateForStatement(node);
            break;
        case ASTNodeKind::ProcedureCall:
            generateProcedureCall(node);
            break;
        case ASTNodeKind::EmptyStatement:
            break;
        default:
            if (isStatementContainer(node.getKind())) {
                generateNode(node);
            } else {
                throw IntermediateCodeGeneratorError("Unsupported statement node: " +
                                                     ASTNode::kindToString(node.getKind()));
            }
            break;
    }
}

void IntermediateCodeGenerator::generateAssignment(const ASTNode &node) {
    const ASTNode *target = requiredChild(node, ASTChildRole::Target);
    const ASTNode *value = requiredChild(node, ASTChildRole::Value);

    generateExpression(*value);
    generateVariableStore(*target);
}

void IntermediateCodeGenerator::generateIfStatement(const ASTNode &node) {
    const ASTNode *condition = requiredChild(node, ASTChildRole::Condition);
    const ASTNode *thenBranch = requiredChild(node, ASTChildRole::Then);
    const ASTNode *elseBranch = node.childWithRole(ASTChildRole::Else);

    generateExpression(*condition);
    std::size_t jumpToElseOrEnd = code_.emitJPC(0, "if false");
    generateStatement(*thenBranch);

    if (elseBranch != nullptr) {
        std::size_t jumpToEnd = code_.emitJMP(0, "end if");
        code_.patchInstructionOperand(jumpToElseOrEnd, static_cast<int>(code_.getNextInstructionIndex()));
        generateStatement(*elseBranch);
        code_.patchInstructionOperand(jumpToEnd, static_cast<int>(code_.getNextInstructionIndex()));
    } else {
        code_.patchInstructionOperand(jumpToElseOrEnd, static_cast<int>(code_.getNextInstructionIndex()));
    }
}

void IntermediateCodeGenerator::generateCaseStatement(const ASTNode &node) {
    const ASTNode *expression = requiredChild(node, ASTChildRole::Expression);
    std::vector<std::size_t> jumpToEnd;

    for (const ASTChild &branchChild : node.getChildren()) {
        if (branchChild.role != ASTChildRole::Branch) {
            continue;
        }

        const ASTNode &branch = branchChild.node;
        std::vector<std::size_t> jumpToBody;
        for (const ASTChild &labelChild : branch.getChildren()) {
            if (labelChild.role != ASTChildRole::Label) {
                continue;
            }
            generateExpression(*expression);
            generateExpression(labelChild.node);
            code_.emitOPR(OperationCode::EQL, "case compare");
            std::size_t jumpToNextLabel = code_.emitJPC(0, "next case label");
            jumpToBody.push_back(code_.emitJMP(0, "case matched"));
            code_.patchInstructionOperand(jumpToNextLabel, static_cast<int>(code_.getNextInstructionIndex()));
        }

        if (!jumpToBody.empty()) {
            const int bodyStart = static_cast<int>(code_.getNextInstructionIndex());
            for (std::size_t jump : jumpToBody) {
                code_.patchInstructionOperand(jump, bodyStart);
            }
        }

        if (!jumpToBody.empty()) {
            bool emittedStatement = false;
            for (const ASTChild &statementChild : branch.getChildren()) {
                if (statementChild.role == ASTChildRole::Statement) {
                    generateStatement(statementChild.node);
                    emittedStatement = true;
                }
            }
            if (emittedStatement) {
                jumpToEnd.push_back(code_.emitJMP(0, "end case"));
            }
        }
    }

    for (std::size_t jump : jumpToEnd) {
        code_.patchInstructionOperand(jump, static_cast<int>(code_.getNextInstructionIndex()));
    }
}

void IntermediateCodeGenerator::generateWhileStatement(const ASTNode &node) {
    const ASTNode *condition = requiredChild(node, ASTChildRole::Condition);
    const ASTNode *body = requiredChild(node, ASTChildRole::Body);

    std::size_t loopStart = code_.getNextInstructionIndex();
    generateExpression(*condition);
    std::size_t jumpToEnd = code_.emitJPC(0, "while false");
    generateStatement(*body);
    code_.emitJMP(static_cast<int>(loopStart), "repeat while");
    code_.patchInstructionOperand(jumpToEnd, static_cast<int>(code_.getNextInstructionIndex()));
}

void IntermediateCodeGenerator::generateRepeatStatement(const ASTNode &node) {
    const ASTNode *body = requiredChild(node, ASTChildRole::Body);
    const ASTNode *condition = requiredChild(node, ASTChildRole::Condition);

    std::size_t loopStart = code_.getNextInstructionIndex();
    generateStatement(*body);
    generateExpression(*condition);
    code_.emitJPC(static_cast<int>(loopStart), "repeat until false");
}

void IntermediateCodeGenerator::generateForStatement(const ASTNode &node) {
    const ASTNode *variable = requiredChild(node, ASTChildRole::Variable);
    const ASTNode *start = requiredChild(node, ASTChildRole::Start);
    const ASTNode *end = requiredChild(node, ASTChildRole::End);
    const ASTNode *body = requiredChild(node, ASTChildRole::Body);
    const std::string direction = lowerCopy(node.getAttribute("direction"));

    generateExpression(*start);
    generateVariableStore(*variable);

    std::size_t loopStart = code_.getNextInstructionIndex();
    generateVariableLoad(*variable);
    generateExpression(*end);
    code_.emitOPR(direction == "downto" ? OperationCode::GEQ : OperationCode::LEQ, "for condition");
    std::size_t jumpToEnd = code_.emitJPC(0, "for false");

    generateStatement(*body);

    generateVariableLoad(*variable);
    code_.emitLIT("1", "for step");
    code_.emitOPR(direction == "downto" ? OperationCode::SUB : OperationCode::ADD, "for update");
    generateVariableStore(*variable);

    code_.emitJMP(static_cast<int>(loopStart), "repeat for");
    code_.patchInstructionOperand(jumpToEnd, static_cast<int>(code_.getNextInstructionIndex()));
}

void IntermediateCodeGenerator::generateProcedureCall(const ASTNode &node) {
    if (!isWriteProcedure(node) && !isWritelnProcedure(node)) {
        int tabIndex = node.annotation().tabIndex;
        if (tabIndex == -1) {
            throw IntermediateCodeGeneratorError("Procedure call requires decorated tab index: " + node.getAttribute("name"));
        }
        const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
        code_.emitCAL(entry.ref, "call " + node.getAttribute("name"));
        return;
    }

    std::vector<const ASTNode *> arguments;
    for (const ASTChild &child : node.getChildren()) {
        if (child.role != ASTChildRole::Arg) {
            continue;
        }
        if (child.node.getKind() == ASTNodeKind::Arguments) {
            for (const ASTChild &argumentChild : child.node.getChildren()) {
                if (argumentChild.role == ASTChildRole::Arg) {
                    arguments.push_back(&argumentChild.node);
                }
            }
        } else {
            arguments.push_back(&child.node);
        }
    }

    if (arguments.empty()) {
        if (isWritelnProcedure(node)) {
            code_.emitLIT("\"\"", "empty writeln");
            code_.emitOPR(OperationCode::WRTLN, "writeln");
        }
        return;
    }

    for (std::size_t i = 0; i < arguments.size(); ++i) {
        generateExpression(*arguments[i]);
        bool isLast = i + 1 == arguments.size();
        code_.emitOPR(isWritelnProcedure(node) && isLast ? OperationCode::WRTLN : OperationCode::WRT,
                      isWritelnProcedure(node) && isLast ? "writeln" : "write");
    }
}

void IntermediateCodeGenerator::generateExpression(const ASTNode &node) {
    switch (node.getKind()) {
        case ASTNodeKind::IntegerLiteral:
        case ASTNodeKind::RealLiteral:
        case ASTNodeKind::CharLiteral:
        case ASTNodeKind::StringLiteral:
        case ASTNodeKind::BooleanLiteral:
            code_.emitLIT(literalValue(node), "literal");
            break;

        case ASTNodeKind::Identifier:
        case ASTNodeKind::Variable:
            generateVariableLoad(node);
            break;

        case ASTNodeKind::BinaryOperation: {
            const ASTNode *left = requiredChild(node, ASTChildRole::Left);
            const ASTNode *right = requiredChild(node, ASTChildRole::Right);
            generateExpression(*left);
            generateExpression(*right);
            code_.emitOPR(binaryOperation(node), "binary " + operatorText(node));
            break;
        }

        case ASTNodeKind::UnaryOperation: {
            const ASTNode *value = node.childWithRole(ASTChildRole::Expression);
            if (value == nullptr) {
                value = node.childWithRole(ASTChildRole::Value);
            }
            if (value == nullptr) {
                throw IntermediateCodeGeneratorError("Unary operation has no operand");
            }

            generateExpression(*value);
            const std::string op = lowerCopy(operatorText(node));
            if (op == "-" || op == "minus") {
                code_.emitOPR(OperationCode::NEG, "unary minus");
            } else if (op == "not" || op == "notsy") {
                code_.emitLIT("false", "not compare");
                code_.emitOPR(OperationCode::EQL, "not");
            }
            break;
        }

        case ASTNodeKind::FunctionCall:
            throw IntermediateCodeGeneratorError("Function call expression generation is not implemented yet: " + node.getAttribute("name"));

        case ASTNodeKind::ArrayAccess:
        case ASTNodeKind::FieldAccess:
            generateVariableLoad(node);
            break;

        default:
            throw IntermediateCodeGeneratorError("Unsupported expression node: " + ASTNode::kindToString(node.getKind()));
    }
}

void IntermediateCodeGenerator::generateVariableLoad(const ASTNode &node) {
    if (node.getKind() != ASTNodeKind::Variable && node.getKind() != ASTNodeKind::Identifier) {
        throw IntermediateCodeGeneratorError("Only simple variable load is supported for now");
    }
    if (node.annotation().tabIndex == -1) {
        const TabEntry *entry = symbolTable_.lookup(node.getAttribute("name"));
        if (entry != nullptr && entry->object == SymbolObjectKind::Constant) {
            code_.emitLIT(entry->value, "constant " + node.getAttribute("name"));
            return;
        }
    }
    code_.emitLOD(nodeAddress(node), "load " + node.getAttribute("name"));
}

void IntermediateCodeGenerator::generateVariableStore(const ASTNode &node) {
    if (node.getKind() != ASTNodeKind::Variable && node.getKind() != ASTNodeKind::Identifier) {
        throw IntermediateCodeGeneratorError("Only simple variable assignment target is supported for now");
    }
    code_.emitSTO(nodeAddress(node), "store " + node.getAttribute("name"));
}

int IntermediateCodeGenerator::frameSize(const ASTNode &programNode) const {
    int blockIndex = programNode.annotation().blockIndex;
    if (blockIndex == -1) {
        blockIndex = 0;
    }

    if (blockIndex >= 0 && blockIndex < static_cast<int>(symbolTable_.btab().size())) {
        return 3 + symbolTable_.btab().at(static_cast<std::size_t>(blockIndex)).variableSize;
    }
    return 3;
}

int IntermediateCodeGenerator::nodeTabIndex(const ASTNode &node) const {
    const int tabIndex = node.annotation().tabIndex;
    if (tabIndex != -1) {
        return tabIndex;
    }

    const std::string name = node.getAttribute("name");
    const int fallbackTabIndex = symbolTable_.lookupIndex(name);
    if (fallbackTabIndex == -1) {
        throw IntermediateCodeGeneratorError("Node has no tab index annotation: " + name);
    }

    return fallbackTabIndex;
}

int IntermediateCodeGenerator::nodeAddress(const ASTNode &node) const {
    const int tabIndex = nodeTabIndex(node);
    return tabAddress(tabIndex);
}

int IntermediateCodeGenerator::tabAddress(int tabIndex) const {
    if (tabIndex < 0 || tabIndex >= static_cast<int>(symbolTable_.tab().size())) {
        throw IntermediateCodeGeneratorError("Invalid tab index: " + std::to_string(tabIndex));
    }
    return 3 + symbolTable_.tab().at(static_cast<std::size_t>(tabIndex)).address;
}

std::string IntermediateCodeGenerator::literalValue(const ASTNode &node) const {
    const std::string value = node.getAttribute("value");
    if (!value.empty()) {
        return value;
    }
    const std::string name = node.getAttribute("name");
    if (!name.empty()) {
        return name;
    }
    return "0";
}

OperationCode IntermediateCodeGenerator::binaryOperation(const ASTNode &node) const {
    const std::string op = lowerCopy(operatorText(node));
    if (op == "+" || op == "plus") return OperationCode::ADD;
    if (op == "-" || op == "minus") return OperationCode::SUB;
    if (op == "*" || op == "times") return OperationCode::MUL;
    if (op == "/" || op == "rdiv" || op == "div") return OperationCode::DIV;
    if (op == "mod" || op == "imod") return OperationCode::MOD;
    if (op == "==" || op == "=" || op == "eql") return OperationCode::EQL;
    if (op == "<>" || op == "!=" || op == "neq") return OperationCode::NEQ;
    if (op == "<" || op == "lss") return OperationCode::LSS;
    if (op == ">=" || op == "geq") return OperationCode::GEQ;
    if (op == ">" || op == "gtr") return OperationCode::GTR;
    if (op == "<=" || op == "leq") return OperationCode::LEQ;
    if (op == "and" || op == "iand") return OperationCode::MUL;
    if (op == "or" || op == "ior") return OperationCode::ADD;
    throw IntermediateCodeGeneratorError("Unsupported binary operator: " + op);
}

std::string IntermediateCodeGenerator::operatorText(const ASTNode &node) const {
    std::string op = node.getAttribute("operator");
    if (op.empty()) {
        op = node.getAttribute("op");
    }
    return op;
}

bool IntermediateCodeGenerator::isWriteProcedure(const ASTNode &node) const {
    return lowerCopy(node.getAttribute("name")) == "write";
}

bool IntermediateCodeGenerator::isWritelnProcedure(const ASTNode &node) const {
    return lowerCopy(node.getAttribute("name")) == "writeln";
}

const ASTNode *IntermediateCodeGenerator::requiredChild(const ASTNode &node, ASTChildRole role) const {
    const ASTNode *child = node.childWithRole(role);
    if (child == nullptr) {
        throw IntermediateCodeGeneratorError(ASTNode::kindToString(node.getKind()) +
                                             " missing child role " +
                                             ASTNode::roleToString(role));
    }
    return child;
}
