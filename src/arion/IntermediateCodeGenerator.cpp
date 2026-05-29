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
    currentBlockIndex_ = 0;
    tabOwnerBlock_.clear();
    tabRuntimeAddress_.clear();
    frameSizes_.clear();
    functionReturnAddress_.clear();
    subprogramEntryLines_.clear();
    pendingCallPatches_.clear();
    prepareRuntimeLayout();
    generateProgram(decoratedAst);
    patchPendingCalls();
    return code_;
}

void IntermediateCodeGenerator::generateProgram(const ASTNode &node) {
    if (node.getKind() != ASTNodeKind::Program) {
        throw IntermediateCodeGeneratorError("Intermediate code generation must start from Program node");
    }

    currentBlockIndex_ = node.annotation().blockIndex == -1 ? 0 : node.annotation().blockIndex;

    std::size_t jumpToMain = code_.emitJMP(0, "program entry");
    generateSubprogramDeclarations(node);
    code_.patchInstructionOperand(jumpToMain, static_cast<int>(code_.getNextInstructionIndex()));

    code_.emitINT(frameSize(node), "init program frame");
    generateNode(node);
    code_.emitRET("return");
}

void IntermediateCodeGenerator::generateSubprogramDeclarations(const ASTNode &node) {
    if (node.getKind() == ASTNodeKind::ProcedureDeclaration ||
        node.getKind() == ASTNodeKind::FunctionDeclaration) {
        generateSubprogram(node);
        return;
    }

    for (const ASTChild &child : node.getChildren()) {
        if (child.role == ASTChildRole::Declaration ||
            child.node.getKind() == ASTNodeKind::Declarations ||
            child.node.getKind() == ASTNodeKind::ConstDeclarations ||
            child.node.getKind() == ASTNodeKind::TypeDeclarations ||
            child.node.getKind() == ASTNodeKind::VarDeclarations ||
            child.node.getKind() == ASTNodeKind::Block) {
            generateSubprogramDeclarations(child.node);
        }
    }
}

void IntermediateCodeGenerator::generateSubprogram(const ASTNode &node) {
    const int tabIndex = node.annotation().tabIndex;
    if (tabIndex < 0 || tabIndex >= static_cast<int>(symbolTable_.tab().size())) {
        throw IntermediateCodeGeneratorError("Subprogram declaration has no valid tab index: " +
                                             node.getAttribute("name"));
    }

    const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
    const int blockIndex = entry.ref;
    if (blockIndex < 0 || blockIndex >= static_cast<int>(symbolTable_.btab().size())) {
        throw IntermediateCodeGeneratorError("Subprogram declaration has invalid block index: " +
                                             node.getAttribute("name"));
    }

    subprogramEntryLines_[blockIndex] = static_cast<int>(code_.getNextInstructionIndex());
    const int previousBlock = currentBlockIndex_;
    currentBlockIndex_ = blockIndex;

    const ASTNode *block = node.childWithRole(ASTChildRole::Block);

    std::size_t jumpToBody = code_.emitJMP(0, "enter " + node.getAttribute("name"));
    if (block != nullptr) {
        generateSubprogramDeclarations(*block);
    }
    code_.patchInstructionOperand(jumpToBody, static_cast<int>(code_.getNextInstructionIndex()));

    code_.emitINT(frameSizeForBlock(blockIndex), "init frame " + node.getAttribute("name"));
    if (block != nullptr) {
        generateNode(*block);
    }

    if (entry.object == SymbolObjectKind::Function) {
        auto returnAddress = functionReturnAddress_.find(blockIndex);
        if (returnAddress == functionReturnAddress_.end()) {
            throw IntermediateCodeGeneratorError("Function has no return slot: " + node.getAttribute("name"));
        }
        code_.emitLOD(0, returnAddress->second, "function result " + node.getAttribute("name"));
    }
    code_.emitRET("return " + node.getAttribute("name"));

    currentBlockIndex_ = previousBlock;
}

void IntermediateCodeGenerator::generateNode(const ASTNode &node) {
    switch (node.getKind()) {
        case ASTNodeKind::Program:
        case ASTNodeKind::Block:
        case ASTNodeKind::CompoundStatement:
        case ASTNodeKind::StatementList: {
            const int previousBlock = currentBlockIndex_;
            if (node.annotation().blockIndex != -1) {
                currentBlockIndex_ = node.annotation().blockIndex;
            }
            for (const ASTChild &child : node.getChildren()) {
                if (child.role == ASTChildRole::Declaration) {
                    continue;
                }
                generateNode(child.node);
            }
            currentBlockIndex_ = previousBlock;
            break;
        }

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
    std::vector<const ASTNode *> arguments = callArguments(node);

    if (isWriteProcedure(node) || isWritelnProcedure(node)) {
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
        return;
    }

    const int tabIndex = nodeTabIndex(node);
    const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
    if (entry.object != SymbolObjectKind::Procedure) {
        throw IntermediateCodeGeneratorError("Only procedures can be called as statements: " +
                                             node.getAttribute("name"));
    }

    for (const ASTNode *argument : arguments) {
        generateExpression(*argument);
    }
    emitCall(entry, node.getAttribute("name"), arguments.size());
}

void IntermediateCodeGenerator::generateFunctionCall(const ASTNode &node) {
    std::vector<const ASTNode *> arguments = callArguments(node);
    const int tabIndex = nodeTabIndex(node);
    const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
    if (entry.object != SymbolObjectKind::Function) {
        throw IntermediateCodeGeneratorError("Only functions can be used as expressions: " +
                                             node.getAttribute("name"));
    }

    for (const ASTNode *argument : arguments) {
        generateExpression(*argument);
    }
    emitCall(entry, node.getAttribute("name"), arguments.size());
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
            const std::string op = lowerCopy(operatorText(node));
            generateExpression(*left);
            generateExpression(*right);
            if (op == "and" || op == "iand" || op == "or" || op == "ior") {
                code_.emitOPR(op == "and" || op == "iand" ? OperationCode::MUL : OperationCode::ADD,
                              "binary " + operatorText(node));
                code_.emitLIT("false", "normalize boolean");
                code_.emitOPR(OperationCode::NEQ, "boolean result");
            } else {
                code_.emitOPR(binaryOperation(node), "binary " + operatorText(node));
            }
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
            generateFunctionCall(node);
            break;

        case ASTNodeKind::ArrayAccess:
            generateVariableLoad(node);
            break;

        case ASTNodeKind::FieldAccess:
            generateVariableLoad(node);
            break;

        default:
            throw IntermediateCodeGeneratorError("Unsupported expression node: " + ASTNode::kindToString(node.getKind()));
    }
}

void IntermediateCodeGenerator::generateVariableLoad(const ASTNode &node) {
    if (node.getKind() != ASTNodeKind::Variable &&
        node.getKind() != ASTNodeKind::Identifier &&
        node.getKind() != ASTNodeKind::FieldAccess &&
        node.getKind() != ASTNodeKind::ArrayAccess) {
        throw IntermediateCodeGeneratorError("Unsupported variable load target: " +
                                             ASTNode::kindToString(node.getKind()));
    }

    if (node.getKind() == ASTNodeKind::Variable || node.getKind() == ASTNodeKind::Identifier) {
        int tabIndex = node.annotation().tabIndex;
        if (tabIndex == -1) {
            tabIndex = lookupVisibleTabIndex(node.getAttribute("name"));
        }
        if (tabIndex < 0 || tabIndex >= static_cast<int>(symbolTable_.tab().size())) {
            throw IntermediateCodeGeneratorError("Node has no tab index annotation: " +
                                                 node.getAttribute("name"));
        }

        const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
        if (entry.object == SymbolObjectKind::Constant) {
            code_.emitLIT(entry.value, "constant " + node.getAttribute("name"));
            return;
        }
        if (isFunctionReturnTarget(tabIndex)) {
            const int functionBlockIndex = entry.ref;
            code_.emitLOD(lexicalLevelDiffToBlock(functionBlockIndex),
                          functionReturnAddress_.at(functionBlockIndex),
                          "load function result " + node.getAttribute("name"));
            return;
        }
    }

    int level = 0;
    int address = 0;
    if (fixedVariableAccessAddress(node, level, address)) {
        code_.emitLOD(level, address, "load " + ASTNode::kindToString(node.getKind()));
        return;
    }

    generateVariableAddress(node);
    code_.emitLDI("load indirect " + ASTNode::kindToString(node.getKind()));
}

void IntermediateCodeGenerator::generateVariableStore(const ASTNode &node) {
    if (node.getKind() != ASTNodeKind::Variable &&
        node.getKind() != ASTNodeKind::Identifier &&
        node.getKind() != ASTNodeKind::FieldAccess &&
        node.getKind() != ASTNodeKind::ArrayAccess) {
        throw IntermediateCodeGeneratorError("Unsupported variable assignment target: " +
                                             ASTNode::kindToString(node.getKind()));
    }

    if ((node.getKind() == ASTNodeKind::Variable || node.getKind() == ASTNodeKind::Identifier) &&
        isFunctionReturnTarget(nodeTabIndex(node))) {
        const int tabIndex = nodeTabIndex(node);
        const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
        const int functionBlockIndex = entry.ref;
        code_.emitSTO(lexicalLevelDiffToBlock(functionBlockIndex),
                      functionReturnAddress_.at(functionBlockIndex),
                      "store function result " + node.getAttribute("name"));
        return;
    }

    int level = 0;
    int address = 0;
    if (fixedVariableAccessAddress(node, level, address)) {
        code_.emitSTO(level, address, "store " + ASTNode::kindToString(node.getKind()));
        return;
    }

    generateVariableAddress(node);
    code_.emitSTI("store indirect " + ASTNode::kindToString(node.getKind()));
}

void IntermediateCodeGenerator::generateVariableAddress(const ASTNode &node) {
    if (node.getKind() == ASTNodeKind::Variable || node.getKind() == ASTNodeKind::Identifier) {
        const int tabIndex = nodeTabIndex(node);
        code_.emitLDA(lexicalLevelDiffForTab(tabIndex), tabAddress(tabIndex),
                      "address " + node.getAttribute("name"));
        return;
    }

    if (node.getKind() == ASTNodeKind::FieldAccess) {
        const ASTNode *base = requiredChild(node, ASTChildRole::Base);
        generateVariableAddress(*base);

        const TabEntry &fieldEntry = symbolTable_.tab().at(static_cast<std::size_t>(fieldTabIndex(node)));
        if (fieldEntry.address != 0) {
            code_.emitLIT(std::to_string(fieldEntry.address), "field offset " + node.getAttribute("field"));
            code_.emitOPR(OperationCode::ADD, "field address");
        }
        return;
    }

    if (node.getKind() == ASTNodeKind::ArrayAccess) {
        const ASTNode *base = requiredChild(node, ASTChildRole::Base);
        auto currentType = nodeValueType(*base);
        generateVariableAddress(*base);

        for (const ASTChild &child : node.getChildren()) {
            if (child.role != ASTChildRole::Index) {
                continue;
            }
            if (currentType.first != TypeKind::Array) {
                throw IntermediateCodeGeneratorError("Too many indices for array access");
            }

            const ATabEntry &arrayEntry = symbolTable_.requireArray(currentType.second);
            generateExpression(child.node);
            code_.emitIXA(arrayEntry.lowOrdinal, arrayEntry.highOrdinal, arrayEntry.elementSize,
                          "array index");
            currentType = {arrayEntry.elementType, arrayEntry.elementRef};
        }
        return;
    }

    throw IntermediateCodeGeneratorError("Unsupported address target: " +
                                         ASTNode::kindToString(node.getKind()));
}

void IntermediateCodeGenerator::prepareRuntimeLayout() {
    for (std::size_t blockIndex = 0; blockIndex < symbolTable_.btab().size(); ++blockIndex) {
        for (int tabIndex : tabEntriesForBlock(static_cast<int>(blockIndex))) {
            tabOwnerBlock_[tabIndex] = static_cast<int>(blockIndex);
        }
    }

    for (std::size_t blockIndex = 0; blockIndex < symbolTable_.btab().size(); ++blockIndex) {
        const BTabEntry &block = symbolTable_.btab().at(blockIndex);
        if (!isActivationBlock(block.kind)) {
            continue;
        }

        int nextAddress = 3 + block.parameterSize;

        for (int tabIndex : tabEntriesForBlock(static_cast<int>(blockIndex))) {
            const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
            if (entry.object == SymbolObjectKind::Parameter) {
                tabRuntimeAddress_[tabIndex] = 3 + entry.address;
            } else if (entry.object == SymbolObjectKind::Variable) {
                tabRuntimeAddress_[tabIndex] = nextAddress + entry.address;
            }
        }
        nextAddress += block.variableSize;

        mapBlockStorage(static_cast<int>(blockIndex), nextAddress);

        if (block.kind == BlockKind::Function) {
            functionReturnAddress_[static_cast<int>(blockIndex)] = nextAddress++;
        }
        frameSizes_[static_cast<int>(blockIndex)] = nextAddress;
    }
}

void IntermediateCodeGenerator::mapBlockStorage(int blockIndex, int &nextAddress) {
    for (std::size_t childIndex = 0; childIndex < symbolTable_.btab().size(); ++childIndex) {
        const BTabEntry &childBlock = symbolTable_.btab().at(childIndex);
        if (childBlock.parent != blockIndex || childBlock.kind != BlockKind::Anonymous) {
            continue;
        }

        for (int tabIndex : tabEntriesForBlock(static_cast<int>(childIndex))) {
            const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
            if (entry.object == SymbolObjectKind::Variable) {
                tabRuntimeAddress_[tabIndex] = nextAddress + entry.address;
            }
        }
        nextAddress += childBlock.variableSize;
        mapBlockStorage(static_cast<int>(childIndex), nextAddress);
    }
}

void IntermediateCodeGenerator::patchPendingCalls() {
    for (const auto &pending : pendingCallPatches_) {
        auto entryLine = subprogramEntryLines_.find(pending.second);
        if (entryLine == subprogramEntryLines_.end()) {
            throw IntermediateCodeGeneratorError("Call target was not generated for block " +
                                                 std::to_string(pending.second));
        }
        code_.patchInstructionOperand(pending.first, entryLine->second);
    }
}

std::vector<int> IntermediateCodeGenerator::tabEntriesForBlock(int blockIndex) const {
    if (blockIndex < 0 || blockIndex >= static_cast<int>(symbolTable_.btab().size())) {
        return {};
    }

    std::vector<int> entries;
    int tabIndex = symbolTable_.btab().at(static_cast<std::size_t>(blockIndex)).last;
    while (tabIndex > 0 && tabIndex < static_cast<int>(symbolTable_.tab().size())) {
        entries.push_back(tabIndex);
        tabIndex = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex)).link;
    }
    std::reverse(entries.begin(), entries.end());
    return entries;
}

std::vector<const ASTNode *> IntermediateCodeGenerator::callArguments(const ASTNode &node) const {
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
    return arguments;
}

int IntermediateCodeGenerator::frameSize(const ASTNode &programNode) const {
    int blockIndex = programNode.annotation().blockIndex;
    if (blockIndex == -1) {
        blockIndex = 0;
    }

    return frameSizeForBlock(blockIndex);
}

int IntermediateCodeGenerator::frameSizeForBlock(int blockIndex) const {
    auto frameSize = frameSizes_.find(blockIndex);
    if (frameSize != frameSizes_.end()) {
        return frameSize->second;
    }
    return 3;
}

int IntermediateCodeGenerator::nodeTabIndex(const ASTNode &node) const {
    const int tabIndex = node.annotation().tabIndex;
    if (tabIndex != -1) {
        return tabIndex;
    }

    const std::string name = node.getAttribute("name");
    const int fallbackTabIndex = lookupVisibleTabIndex(name);
    if (fallbackTabIndex == -1) {
        throw IntermediateCodeGeneratorError("Node has no tab index annotation: " + name);
    }

    return fallbackTabIndex;
}

int IntermediateCodeGenerator::nodeAddress(const ASTNode &node) const {
    const int tabIndex = nodeTabIndex(node);
    return tabAddress(tabIndex);
}

std::pair<int, int> IntermediateCodeGenerator::variableAccessAddress(const ASTNode &node) const {
    int level = 0;
    int address = 0;
    if (fixedVariableAccessAddress(node, level, address)) {
        return {level, address};
    }

    throw IntermediateCodeGeneratorError("Unsupported variable access address: " +
                                         ASTNode::kindToString(node.getKind()));
}

bool IntermediateCodeGenerator::fixedVariableAccessAddress(const ASTNode &node, int &level, int &address) const {
    if (node.getKind() == ASTNodeKind::Variable || node.getKind() == ASTNodeKind::Identifier) {
        const int tabIndex = nodeTabIndex(node);
        const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
        if (entry.object != SymbolObjectKind::Variable &&
            entry.object != SymbolObjectKind::Parameter) {
            return false;
        }
        level = lexicalLevelDiffForTab(tabIndex);
        address = tabAddress(tabIndex);
        return true;
    }

    if (node.getKind() == ASTNodeKind::FieldAccess) {
        const ASTNode *base = requiredChild(node, ASTChildRole::Base);
        if (!fixedVariableAccessAddress(*base, level, address)) {
            return false;
        }
        const TabEntry &fieldEntry = symbolTable_.tab().at(static_cast<std::size_t>(fieldTabIndex(node)));
        address += fieldEntry.address;
        return true;
    }

    if (node.getKind() == ASTNodeKind::ArrayAccess) {
        const ASTNode *base = requiredChild(node, ASTChildRole::Base);
        if (!fixedVariableAccessAddress(*base, level, address)) {
            return false;
        }

        auto currentType = nodeValueType(*base);
        int offset = 0;
        bool hasIndex = false;
        for (const ASTChild &child : node.getChildren()) {
            if (child.role != ASTChildRole::Index) {
                continue;
            }
            hasIndex = true;
            if (currentType.first != TypeKind::Array) {
                throw IntermediateCodeGeneratorError("Too many indices for array access");
            }

            int ordinal = 0;
            if (!staticIndexOrdinal(child.node, ordinal)) {
                return false;
            }

            const ATabEntry &arrayEntry = symbolTable_.requireArray(currentType.second);
            if (ordinal < arrayEntry.lowOrdinal || ordinal > arrayEntry.highOrdinal) {
                throw IntermediateCodeGeneratorError("Static array index out of bounds: " +
                                                     std::to_string(ordinal) + " not in " +
                                                     std::to_string(arrayEntry.lowOrdinal) + ".." +
                                                     std::to_string(arrayEntry.highOrdinal));
            }
            offset += (ordinal - arrayEntry.lowOrdinal) * arrayEntry.elementSize;
            currentType = {arrayEntry.elementType, arrayEntry.elementRef};
        }

        if (!hasIndex) {
            return false;
        }
        address += offset;
        return true;
    }

    return false;
}

std::pair<TypeKind, int> IntermediateCodeGenerator::nodeValueType(const ASTNode &node) const {
    if (node.getKind() == ASTNodeKind::Variable || node.getKind() == ASTNodeKind::Identifier) {
        const int tabIndex = nodeTabIndex(node);
        const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
        return {entry.type, entry.ref};
    }

    if (node.getKind() == ASTNodeKind::FieldAccess) {
        const int tabIndex = fieldTabIndex(node);
        const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
        return {entry.type, entry.ref};
    }

    if (node.getKind() == ASTNodeKind::ArrayAccess) {
        const ASTNode *base = requiredChild(node, ASTChildRole::Base);
        auto currentType = nodeValueType(*base);
        for (const ASTChild &child : node.getChildren()) {
            if (child.role != ASTChildRole::Index) {
                continue;
            }
            if (currentType.first != TypeKind::Array) {
                throw IntermediateCodeGeneratorError("Too many indices for array access");
            }
            const ATabEntry &arrayEntry = symbolTable_.requireArray(currentType.second);
            currentType = {arrayEntry.elementType, arrayEntry.elementRef};
        }
        return currentType;
    }

    throw IntermediateCodeGeneratorError("Cannot determine value type for " +
                                         ASTNode::kindToString(node.getKind()));
}

int IntermediateCodeGenerator::fieldTabIndex(const ASTNode &node) const {
    if (node.getKind() != ASTNodeKind::FieldAccess) {
        throw IntermediateCodeGeneratorError("Expected field access node");
    }

    const int annotatedTabIndex = node.annotation().tabIndex;
    if (annotatedTabIndex >= 0 && annotatedTabIndex < static_cast<int>(symbolTable_.tab().size())) {
        return annotatedTabIndex;
    }

    const ASTNode *base = requiredChild(node, ASTChildRole::Base);
    auto baseType = nodeValueType(*base);
    if (baseType.first != TypeKind::Record) {
        throw IntermediateCodeGeneratorError("Field access base is not a record: " +
                                             node.getAttribute("field"));
    }

    const int recordBlock = baseType.second;
    if (recordBlock < 0 || recordBlock >= static_cast<int>(symbolTable_.btab().size())) {
        throw IntermediateCodeGeneratorError("Field access has invalid record block: " +
                                             node.getAttribute("field"));
    }

    for (int tabIndex : tabEntriesForBlock(recordBlock)) {
        const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
        if (entry.object == SymbolObjectKind::Field &&
            sameIdentifier(entry.identifier, node.getAttribute("field"))) {
            return tabIndex;
        }
    }

    throw IntermediateCodeGeneratorError("Record field not found: " + node.getAttribute("field"));
}

bool IntermediateCodeGenerator::staticIndexOrdinal(const ASTNode &node, int &ordinal) const {
    if (node.getKind() == ASTNodeKind::IntegerLiteral) {
        return literalOrdinal(TypeKind::Integer, literalValue(node), ordinal);
    }
    if (node.getKind() == ASTNodeKind::BooleanLiteral) {
        return literalOrdinal(TypeKind::Boolean, literalValue(node), ordinal);
    }
    if (node.getKind() == ASTNodeKind::CharLiteral) {
        return literalOrdinal(TypeKind::Char, literalValue(node), ordinal);
    }
    if (node.getKind() == ASTNodeKind::Variable || node.getKind() == ASTNodeKind::Identifier) {
        int tabIndex = node.annotation().tabIndex;
        if (tabIndex == -1) {
            tabIndex = lookupVisibleTabIndex(node.getAttribute("name"));
        }
        if (tabIndex < 0 || tabIndex >= static_cast<int>(symbolTable_.tab().size())) {
            return false;
        }

        const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
        if (entry.object != SymbolObjectKind::Constant) {
            return false;
        }
        return literalOrdinal(entry.type, entry.value, ordinal);
    }
    return false;
}

bool IntermediateCodeGenerator::literalOrdinal(TypeKind type, const std::string &value, int &ordinal) const {
    try {
        if (type == TypeKind::Integer || type == TypeKind::Enumerated) {
            std::size_t parsed = 0;
            ordinal = std::stoi(value, &parsed);
            return parsed == value.size();
        }
        if (type == TypeKind::Boolean) {
            const std::string lowered = lowerCopy(value);
            if (lowered == "false" || value == "0") {
                ordinal = 0;
                return true;
            }
            if (lowered == "true" || value == "1") {
                ordinal = 1;
                return true;
            }
            return false;
        }
        if (type == TypeKind::Char) {
            if (value.size() == 1) {
                ordinal = static_cast<unsigned char>(value[0]);
                return true;
            }
            if (value.size() >= 3 && value.front() == '\'' && value.back() == '\'') {
                ordinal = static_cast<unsigned char>(value[1]);
                return true;
            }
        }
    } catch (...) {
        return false;
    }
    return false;
}

int IntermediateCodeGenerator::tabAddress(int tabIndex) const {
    if (tabIndex < 0 || tabIndex >= static_cast<int>(symbolTable_.tab().size())) {
        throw IntermediateCodeGeneratorError("Invalid tab index: " + std::to_string(tabIndex));
    }
    auto runtimeAddress = tabRuntimeAddress_.find(tabIndex);
    if (runtimeAddress == tabRuntimeAddress_.end()) {
        throw IntermediateCodeGeneratorError("Identifier has no runtime address: " +
                                             symbolTable_.tab().at(static_cast<std::size_t>(tabIndex)).identifier);
    }
    return runtimeAddress->second;
}

int IntermediateCodeGenerator::tabBlockIndex(int tabIndex) const {
    auto owner = tabOwnerBlock_.find(tabIndex);
    if (owner == tabOwnerBlock_.end()) {
        throw IntermediateCodeGeneratorError("Identifier has no owning block: " +
                                             std::to_string(tabIndex));
    }
    return owner->second;
}

int IntermediateCodeGenerator::activationBlockIndex(int blockIndex) const {
    int current = blockIndex;
    while (current >= 0 && current < static_cast<int>(symbolTable_.btab().size())) {
        const BTabEntry &block = symbolTable_.btab().at(static_cast<std::size_t>(current));
        if (isActivationBlock(block.kind)) {
            return current;
        }
        current = block.parent;
    }
    return 0;
}

int IntermediateCodeGenerator::lexicalLevelDiffToBlock(int targetActivationBlockIndex) const {
    int blockIndex = activationBlockIndex(currentBlockIndex_);
    int diff = 0;
    while (blockIndex >= 0 && blockIndex < static_cast<int>(symbolTable_.btab().size())) {
        if (blockIndex == targetActivationBlockIndex) {
            return diff;
        }
        const int parent = symbolTable_.btab().at(static_cast<std::size_t>(blockIndex)).parent;
        if (parent < 0) {
            break;
        }
        blockIndex = activationBlockIndex(parent);
        ++diff;
    }

    throw IntermediateCodeGeneratorError("Target activation block is not visible: " +
                                         std::to_string(targetActivationBlockIndex));
}

int IntermediateCodeGenerator::lexicalLevelDiffForTab(int tabIndex) const {
    return lexicalLevelDiffToBlock(activationBlockIndex(tabBlockIndex(tabIndex)));
}

int IntermediateCodeGenerator::callableLevelDiff(int calleeBlockIndex) const {
    if (calleeBlockIndex < 0 || calleeBlockIndex >= static_cast<int>(symbolTable_.btab().size())) {
        throw IntermediateCodeGeneratorError("Invalid callable block: " + std::to_string(calleeBlockIndex));
    }

    const int parentBlock = symbolTable_.btab().at(static_cast<std::size_t>(calleeBlockIndex)).parent;
    const int staticTargetBlock = activationBlockIndex(parentBlock);
    return lexicalLevelDiffToBlock(staticTargetBlock);
}

int IntermediateCodeGenerator::lookupVisibleTabIndex(const std::string &name) const {
    int blockIndex = currentBlockIndex_;
    while (blockIndex >= 0 && blockIndex < static_cast<int>(symbolTable_.btab().size())) {
        int tabIndex = symbolTable_.btab().at(static_cast<std::size_t>(blockIndex)).last;
        while (tabIndex > 0 && tabIndex < static_cast<int>(symbolTable_.tab().size())) {
            if (sameIdentifier(symbolTable_.tab().at(static_cast<std::size_t>(tabIndex)).identifier, name)) {
                return tabIndex;
            }
            tabIndex = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex)).link;
        }
        blockIndex = symbolTable_.btab().at(static_cast<std::size_t>(blockIndex)).parent;
    }
    return -1;
}

int IntermediateCodeGenerator::emitCall(const TabEntry &entry, const std::string &name, std::size_t argumentCount) {
    const int calleeBlockIndex = entry.ref;
    code_.emitLIT(std::to_string(argumentCount), "argument count");

    auto entryLine = subprogramEntryLines_.find(calleeBlockIndex);
    const int line = entryLine == subprogramEntryLines_.end() ? 0 : entryLine->second;
    const std::size_t callInstruction = code_.emitCAL(callableLevelDiff(calleeBlockIndex), line, "call " + name);
    if (entryLine == subprogramEntryLines_.end()) {
        pendingCallPatches_.push_back({callInstruction, calleeBlockIndex});
    }
    return static_cast<int>(callInstruction);
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

std::string IntermediateCodeGenerator::normalizeIdentifier(const std::string &identifier) const {
    return lowerCopy(identifier);
}

bool IntermediateCodeGenerator::sameIdentifier(const std::string &left, const std::string &right) const {
    return normalizeIdentifier(left) == normalizeIdentifier(right);
}

bool IntermediateCodeGenerator::isWriteProcedure(const ASTNode &node) const {
    return lowerCopy(node.getAttribute("name")) == "write";
}

bool IntermediateCodeGenerator::isWritelnProcedure(const ASTNode &node) const {
    return lowerCopy(node.getAttribute("name")) == "writeln";
}

bool IntermediateCodeGenerator::isActivationBlock(BlockKind kind) const {
    return kind == BlockKind::Program ||
           kind == BlockKind::Procedure ||
           kind == BlockKind::Function;
}

bool IntermediateCodeGenerator::isFunctionReturnTarget(int tabIndex) const {
    if (tabIndex < 0 || tabIndex >= static_cast<int>(symbolTable_.tab().size())) {
        return false;
    }

    const TabEntry &entry = symbolTable_.tab().at(static_cast<std::size_t>(tabIndex));
    if (entry.object != SymbolObjectKind::Function) {
        return false;
    }
    if (functionReturnAddress_.find(entry.ref) == functionReturnAddress_.end()) {
        return false;
    }

    const int currentActivation = activationBlockIndex(currentBlockIndex_);
    int blockIndex = currentActivation;
    while (blockIndex >= 0 && blockIndex < static_cast<int>(symbolTable_.btab().size())) {
        if (blockIndex == entry.ref) {
            return true;
        }
        blockIndex = symbolTable_.btab().at(static_cast<std::size_t>(blockIndex)).parent;
    }
    return false;
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
