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
        default:
            if (isStatementContainer(node.getKind())) {
                generateNode(node);
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
