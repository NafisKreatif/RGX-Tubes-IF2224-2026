#include "ASTBuilder.hpp"
#include "Tokenizer.hpp"
#include <cctype>
#include <utility>

using namespace arion;

namespace {
    std::string lowerCopy(std::string value) {
        for (char &c : value) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return value;
    }

    bool isNonTerminalLabel(const ParseNode &node, const std::string &label) {
        return !node.getSymbol().isTerminal() && node.getLabel() == label;
    }
}

ASTBuilderError::ASTBuilderError(const std::string &message) : std::runtime_error(message) {}

ASTNode ASTBuilder::build(const ParseNode &parseTree) const {
    return buildNode(parseTree);
}

ASTNode ASTBuilder::buildNode(const ParseNode &node) const {
    const std::string label = node.getLabel();

    if (label == "<program>") return buildProgram(node);
    if (label == "<declaration-part>") return buildDeclarationPart(node);
    if (label == "<const-declaration>") return buildConstDeclaration(node);
    if (label == "<type-declaration>") return buildTypeDeclaration(node);
    if (label == "<var-declaration>") return buildVarDeclaration(node);
    if (label == "<type>") return buildType(node);
    if (label == "<array-type>") return buildArrayType(node);
    if (label == "<range>") return buildRange(node);
    if (label == "<enumerated>") return buildEnumerated(node);
    if (label == "<record-type>") return buildRecordType(node);
    if (label == "<field-list>") return buildFieldList(node);
    if (label == "<field-part>") return buildFieldPart(node);
    if (label == "<subprogram-declaration>") return buildSubprogramDeclaration(node);
    if (label == "<procedure-declaration>") return buildProcedureDeclaration(node);
    if (label == "<function-declaration>") return buildFunctionDeclaration(node);
    if (label == "<block>") return buildBlock(node);
    if (label == "<formal-parameter-list>") return buildFormalParameterList(node);
    if (label == "<parameter-group>") return buildParameterGroup(node);
    if (label == "<compound-statement>") return buildCompoundStatement(node);
    if (label == "<statement-list>") return buildStatementList(node);
    if (label == "<statement>") return buildStatement(node);
    if (label == "<assignment-statement>") return buildAssignmentStatement(node);
    if (label == "<if-statement>") return buildIfStatement(node);
    if (label == "<case-statement>") return buildCaseStatement(node);
    if (label == "<case-block>") {
        std::vector<ASTNode> branches = buildCaseBranches(node);
        return branches.empty() ? ASTNode(ASTNodeKind::Unknown) : std::move(branches.front());
    }
    if (label == "<while-statement>") return buildWhileStatement(node);
    if (label == "<repeat-statement>") return buildRepeatStatement(node);
    if (label == "<for-statement>") return buildForStatement(node);
    if (label == "<procedure/function-call>") return buildProcedureOrFunctionCall(node, false);
    if (label == "<parameter-list>") return buildParameterList(node);
    if (label == "<expression>") return buildExpression(node);
    if (label == "<simple-expression>") return buildSimpleExpression(node);
    if (label == "<term>") return buildTerm(node);
    if (label == "<factor>") return buildFactor(node);
    if (label == "<variable>") return buildVariable(node);
    if (label == "<index-list>") {
        std::vector<ASTNode> indexes = buildIndexList(node);
        ASTNode arguments(ASTNodeKind::Arguments);
        for (ASTNode &index : indexes) {
            arguments.addChild(ASTChildRole::Index, std::move(index));
        }
        return arguments;
    }
    if (label == "<constant>") return buildConstant(node);

    if (node.getSymbol().isTerminal()) {
        return terminalAsLiteralOrIdentifier(node);
    }

    ASTNode unknown(ASTNodeKind::Unknown);
    unknown.setAttribute("parse_node", label);
    for (const ParseNode &child : node.getChildren()) {
        if (!child.getSymbol().isTerminal()) {
            unknown.addChild(buildNode(child));
        }
    }
    return unknown;
}

ASTNode ASTBuilder::buildProgram(const ParseNode &node) const {
    ASTNode program(ASTNodeKind::Program);

    const ParseNode *header = firstChildWithLabel(node, "<program-header>");
    if (header != nullptr) {
        for (const ParseNode &child : header->getChildren()) {
            if (isTerminal(child, Tokenizer::TOKEN_IDENT)) {
                program.setAttribute("name", terminalValue(child));
                break;
            }
        }
    }

    if (const ParseNode *declarations = firstChildWithLabel(node, "<declaration-part>")) {
        program.addChild(ASTChildRole::Declaration, buildDeclarationPart(*declarations));
    }
    if (const ParseNode *body = firstChildWithLabel(node, "<compound-statement>")) {
        program.addChild(ASTChildRole::Body, buildCompoundStatement(*body));
    }
    return program;
}

ASTNode ASTBuilder::buildDeclarationPart(const ParseNode &node) const {
    ASTNode declarations(ASTNodeKind::Declarations);
    for (const ParseNode &child : node.getChildren()) {
        if (!child.getSymbol().isTerminal()) {
            declarations.addChild(ASTChildRole::Declaration, buildNode(child));
        }
    }
    return declarations;
}

ASTNode ASTBuilder::buildConstDeclaration(const ParseNode &node) const {
    ASTNode declarations(ASTNodeKind::ConstDeclarations);
    const auto &children = node.getChildren();

    for (std::size_t i = 0; i < children.size(); ++i) {
        if (!isTerminal(children[i], Tokenizer::TOKEN_IDENT)) continue;

        ASTNode declaration(ASTNodeKind::ConstDeclaration);
        declaration.setAttribute("name", terminalValue(children[i]));

        for (std::size_t j = i + 1; j < children.size(); ++j) {
            if (isNonTerminalLabel(children[j], "<constant>")) {
                declaration.addChild(ASTChildRole::Value, buildConstant(children[j]));
                break;
            }
            if (isTerminal(children[j], Tokenizer::TOKEN_SEMICOLON)) break;
        }
        declarations.addChild(ASTChildRole::Const, std::move(declaration));
    }
    return declarations;
}

ASTNode ASTBuilder::buildTypeDeclaration(const ParseNode &node) const {
    ASTNode declarations(ASTNodeKind::TypeDeclarations);
    const auto &children = node.getChildren();

    for (std::size_t i = 0; i < children.size(); ++i) {
        if (!isTerminal(children[i], Tokenizer::TOKEN_IDENT)) continue;

        ASTNode declaration(ASTNodeKind::TypeDeclaration);
        declaration.setAttribute("name", terminalValue(children[i]));
        for (std::size_t j = i + 1; j < children.size(); ++j) {
            if (isNonTerminalLabel(children[j], "<type>")) {
                declaration.addChild(ASTChildRole::Type, buildType(children[j]));
                break;
            }
            if (isTerminal(children[j], Tokenizer::TOKEN_SEMICOLON)) break;
        }
        declarations.addChild(ASTChildRole::Type, std::move(declaration));
    }
    return declarations;
}

ASTNode ASTBuilder::buildVarDeclaration(const ParseNode &node) const {
    ASTNode declarations(ASTNodeKind::VarDeclarations);
    const auto &children = node.getChildren();

    for (std::size_t i = 0; i < children.size(); ++i) {
        if (!isNonTerminalLabel(children[i], "<identifier-list>")) continue;

        std::vector<std::string> names = collectIdentifierList(children[i]);
        const ParseNode *typeNode = nullptr;
        for (std::size_t j = i + 1; j < children.size(); ++j) {
            if (isNonTerminalLabel(children[j], "<type>")) {
                typeNode = &children[j];
                break;
            }
            if (isTerminal(children[j], Tokenizer::TOKEN_SEMICOLON)) break;
        }
        if (typeNode == nullptr) continue;

        for (const std::string &name : names) {
            ASTNode declaration(ASTNodeKind::VarDeclaration);
            declaration.setAttribute("name", name);
            declaration.addChild(ASTChildRole::Type, buildType(*typeNode));
            declarations.addChild(ASTChildRole::Variable, std::move(declaration));
        }
    }
    return declarations;
}

ASTNode ASTBuilder::buildType(const ParseNode &node) const {
    for (const ParseNode &child : node.getChildren()) {
        if (isTerminal(child, Tokenizer::TOKEN_IDENT)) {
            ASTNode type(ASTNodeKind::NamedType);
            type.setAttribute("name", terminalValue(child));
            return type;
        }
        if (!child.getSymbol().isTerminal()) {
            return buildNode(child);
        }
    }
    return ASTNode(ASTNodeKind::Unknown);
}

ASTNode ASTBuilder::buildArrayType(const ParseNode &node) const {
    ASTNode arrayType(ASTNodeKind::ArrayType);
    for (const ParseNode &child : node.getChildren()) {
        if (isNonTerminalLabel(child, "<range>")) {
            arrayType.addChild(ASTChildRole::Index, buildRange(child));
        } else if (isNonTerminalLabel(child, "<type>")) {
            arrayType.addChild(ASTChildRole::Element, buildType(child));
        } else if (isTerminal(child, Tokenizer::TOKEN_IDENT)) {
            arrayType.addChild(ASTChildRole::Index, makeIdentifier(terminalValue(child)));
        }
    }
    return arrayType;
}

ASTNode ASTBuilder::buildRange(const ParseNode &node) const {
    ASTNode range(ASTNodeKind::RangeType);
    bool lowFilled = false;
    for (const ParseNode &child : node.getChildren()) {
        if (!isNonTerminalLabel(child, "<constant>")) continue;

        if (!lowFilled) {
            range.addChild(ASTChildRole::Low, buildConstant(child));
            lowFilled = true;
        } else {
            range.addChild(ASTChildRole::High, buildConstant(child));
            break;
        }
    }
    return range;
}

ASTNode ASTBuilder::buildEnumerated(const ParseNode &node) const {
    ASTNode enumerated(ASTNodeKind::EnumeratedType);
    for (const ParseNode &child : node.getChildren()) {
        if (isTerminal(child, Tokenizer::TOKEN_IDENT)) {
            enumerated.addChild(ASTChildRole::Element, makeIdentifier(terminalValue(child)));
        }
    }
    return enumerated;
}

ASTNode ASTBuilder::buildRecordType(const ParseNode &node) const {
    ASTNode record(ASTNodeKind::RecordType);
    if (const ParseNode *fields = firstChildWithLabel(node, "<field-list>")) {
        ASTNode fieldList = buildFieldList(*fields);
        for (ASTChild child : fieldList.getChildren()) {
            record.addChild(ASTChildRole::Field, std::move(child.node));
        }
    }
    return record;
}

ASTNode ASTBuilder::buildFieldList(const ParseNode &node) const {
    ASTNode fields(ASTNodeKind::RecordType);
    for (const ParseNode &child : node.getChildren()) {
        if (isNonTerminalLabel(child, "<field-part>")) {
            ASTNode fieldPart = buildFieldPart(child);
            for (ASTChild field : fieldPart.getChildren()) {
                fields.addChild(ASTChildRole::Field, std::move(field.node));
            }
        }
    }
    return fields;
}

ASTNode ASTBuilder::buildFieldPart(const ParseNode &node) const {
    ASTNode fields(ASTNodeKind::RecordType);
    const ParseNode *identifierList = firstChildWithLabel(node, "<identifier-list>");
    const ParseNode *typeNode = firstChildWithLabel(node, "<type>");
    if (identifierList == nullptr || typeNode == nullptr) return fields;

    for (const std::string &name : collectIdentifierList(*identifierList)) {
        ASTNode field(ASTNodeKind::FieldDeclaration);
        field.setAttribute("name", name);
        field.addChild(ASTChildRole::Type, buildType(*typeNode));
        fields.addChild(ASTChildRole::Field, std::move(field));
    }
    return fields;
}

ASTNode ASTBuilder::buildSubprogramDeclaration(const ParseNode &node) const {
    for (const ParseNode &child : node.getChildren()) {
        if (!child.getSymbol().isTerminal()) return buildNode(child);
    }
    return ASTNode(ASTNodeKind::Unknown);
}

ASTNode ASTBuilder::buildProcedureDeclaration(const ParseNode &node) const {
    ASTNode procedure(ASTNodeKind::ProcedureDeclaration);
    for (const ParseNode &child : node.getChildren()) {
        if (isTerminal(child, Tokenizer::TOKEN_IDENT)) {
            procedure.setAttribute("name", terminalValue(child));
            break;
        }
    }
    if (const ParseNode *parameters = firstChildWithLabel(node, "<formal-parameter-list>")) {
        procedure.addChild(ASTChildRole::Parameters, buildFormalParameterList(*parameters));
    }
    if (const ParseNode *block = firstChildWithLabel(node, "<block>")) {
        procedure.addChild(ASTChildRole::Block, buildBlock(*block));
    }
    return procedure;
}

ASTNode ASTBuilder::buildFunctionDeclaration(const ParseNode &node) const {
    ASTNode function(ASTNodeKind::FunctionDeclaration);
    bool nameFilled = false;
    bool returnTypeFilled = false;

    for (const ParseNode &child : node.getChildren()) {
        if (isTerminal(child, Tokenizer::TOKEN_IDENT)) {
            if (!nameFilled) {
                function.setAttribute("name", terminalValue(child));
                nameFilled = true;
            } else if (!returnTypeFilled) {
                ASTNode returnType(ASTNodeKind::ReturnType);
                returnType.addChild(ASTChildRole::Type, makeIdentifier(terminalValue(child)));
                function.addChild(ASTChildRole::ReturnType, std::move(returnType));
                returnTypeFilled = true;
            }
        }
    }
    if (const ParseNode *parameters = firstChildWithLabel(node, "<formal-parameter-list>")) {
        function.addChild(ASTChildRole::Parameters, buildFormalParameterList(*parameters));
    }
    if (const ParseNode *block = firstChildWithLabel(node, "<block>")) {
        function.addChild(ASTChildRole::Block, buildBlock(*block));
    }
    return function;
}

ASTNode ASTBuilder::buildBlock(const ParseNode &node) const {
    ASTNode block(ASTNodeKind::Block);
    if (const ParseNode *declarations = firstChildWithLabel(node, "<declaration-part>")) {
        block.addChild(ASTChildRole::Declaration, buildDeclarationPart(*declarations));
    }
    if (const ParseNode *body = firstChildWithLabel(node, "<compound-statement>")) {
        block.addChild(ASTChildRole::Body, buildCompoundStatement(*body));
    }
    return block;
}

ASTNode ASTBuilder::buildFormalParameterList(const ParseNode &node) const {
    ASTNode parameters(ASTNodeKind::Parameters);
    for (const ParseNode &child : node.getChildren()) {
        if (isNonTerminalLabel(child, "<parameter-group>")) {
            parameters.addChild(ASTChildRole::Group, buildParameterGroup(child));
        }
    }
    return parameters;
}

ASTNode ASTBuilder::buildParameterGroup(const ParseNode &node) const {
    ASTNode group(ASTNodeKind::ParameterGroup);
    const ParseNode *identifierList = firstChildWithLabel(node, "<identifier-list>");
    if (identifierList == nullptr) return group;

    const ParseNode *typeNode = nullptr;
    for (const ParseNode &child : node.getChildren()) {
        if (isTerminal(child, Tokenizer::TOKEN_IDENT) && !isNonTerminalLabel(child, "<identifier-list>")) {
            continue;
        }
        if (isNonTerminalLabel(child, "<array-type>")) {
            typeNode = &child;
            break;
        }
    }

    for (const ParseNode &child : node.getChildren()) {
        if (isTerminal(child, Tokenizer::TOKEN_IDENT)) {
            bool isParameterName = false;
            for (const std::string &name : collectIdentifierList(*identifierList)) {
                if (name == terminalValue(child)) {
                    isParameterName = true;
                    break;
                }
            }
            if (!isParameterName) {
                typeNode = &child;
                break;
            }
        }
    }

    for (const std::string &name : collectIdentifierList(*identifierList)) {
        ASTNode parameter(ASTNodeKind::Parameter);
        parameter.setAttribute("name", name);
        if (typeNode != nullptr) {
            if (typeNode->getSymbol().isTerminal()) {
                ASTNode type(ASTNodeKind::NamedType);
                type.setAttribute("name", terminalValue(*typeNode));
                parameter.addChild(ASTChildRole::Type, std::move(type));
            } else {
                parameter.addChild(ASTChildRole::Type, buildNode(*typeNode));
            }
        }
        group.addChild(ASTChildRole::Parameter, std::move(parameter));
    }
    return group;
}

ASTNode ASTBuilder::buildCompoundStatement(const ParseNode &node) const {
    ASTNode compound(ASTNodeKind::CompoundStatement);
    if (const ParseNode *statements = firstChildWithLabel(node, "<statement-list>")) {
        compound.addChild(ASTChildRole::Body, buildStatementList(*statements));
    }
    return compound;
}

ASTNode ASTBuilder::buildStatementList(const ParseNode &node) const {
    ASTNode statements(ASTNodeKind::StatementList);
    for (const ParseNode &child : node.getChildren()) {
        if (isNonTerminalLabel(child, "<statement>")) {
            statements.addChild(ASTChildRole::Statement, buildStatement(child));
        }
    }
    return statements;
}

ASTNode ASTBuilder::buildStatement(const ParseNode &node) const {
    for (const ParseNode &child : node.getChildren()) {
        if (!child.getSymbol().isTerminal()) {
            if (isNonTerminalLabel(child, "<procedure/function-call>")) {
                return buildProcedureOrFunctionCall(child, false);
            }
            return buildNode(child);
        }
    }
    return ASTNode(ASTNodeKind::EmptyStatement);
}

ASTNode ASTBuilder::buildAssignmentStatement(const ParseNode &node) const {
    ASTNode assignment(ASTNodeKind::Assignment);
    if (const ParseNode *variable = firstChildWithLabel(node, "<variable>")) {
        assignment.addChild(ASTChildRole::Target, buildVariable(*variable));
    }
    if (const ParseNode *expression = firstChildWithLabel(node, "<expression>")) {
        assignment.addChild(ASTChildRole::Value, buildExpression(*expression));
    }
    return assignment;
}

ASTNode ASTBuilder::buildIfStatement(const ParseNode &node) const {
    ASTNode ifStatement(ASTNodeKind::IfStatement);
    bool conditionFilled = false;
    bool thenFilled = false;

    for (const ParseNode &child : node.getChildren()) {
        if (isNonTerminalLabel(child, "<expression>") && !conditionFilled) {
            ifStatement.addChild(ASTChildRole::Condition, buildExpression(child));
            conditionFilled = true;
        } else if (isNonTerminalLabel(child, "<statement>")) {
            if (!thenFilled) {
                ifStatement.addChild(ASTChildRole::Then, buildStatement(child));
                thenFilled = true;
            } else {
                ifStatement.addChild(ASTChildRole::Else, buildStatement(child));
            }
        }
    }
    return ifStatement;
}

ASTNode ASTBuilder::buildCaseStatement(const ParseNode &node) const {
    ASTNode caseStatement(ASTNodeKind::CaseStatement);
    if (const ParseNode *expression = firstChildWithLabel(node, "<expression>")) {
        caseStatement.addChild(ASTChildRole::Expression, buildExpression(*expression));
    }
    for (const ParseNode &child : node.getChildren()) {
        if (isNonTerminalLabel(child, "<case-block>")) {
            for (ASTNode &branch : buildCaseBranches(child)) {
                caseStatement.addChild(ASTChildRole::Branch, std::move(branch));
            }
        }
    }
    return caseStatement;
}

std::vector<ASTNode> ASTBuilder::buildCaseBranches(const ParseNode &node) const {
    std::vector<ASTNode> branches;
    ASTNode branch(ASTNodeKind::CaseBranch);

    for (const ParseNode &child : node.getChildren()) {
        if (isNonTerminalLabel(child, "<constant>")) {
            branch.addChild(ASTChildRole::Label, buildConstant(child));
        } else if (isNonTerminalLabel(child, "<statement>")) {
            branch.addChild(ASTChildRole::Statement, buildStatement(child));
        } else if (isNonTerminalLabel(child, "<case-block>")) {
            branches.push_back(std::move(branch));
            std::vector<ASTNode> nested = buildCaseBranches(child);
            branches.insert(branches.end(), std::make_move_iterator(nested.begin()), std::make_move_iterator(nested.end()));
            return branches;
        }
    }

    branches.push_back(std::move(branch));
    return branches;
}

ASTNode ASTBuilder::buildWhileStatement(const ParseNode &node) const {
    ASTNode whileStatement(ASTNodeKind::WhileStatement);
    if (const ParseNode *condition = firstChildWithLabel(node, "<expression>")) {
        whileStatement.addChild(ASTChildRole::Condition, buildExpression(*condition));
    }
    if (const ParseNode *body = firstChildWithLabel(node, "<compound-statement>")) {
        whileStatement.addChild(ASTChildRole::Body, buildCompoundStatement(*body));
    } else if (const ParseNode *statement = firstChildWithLabel(node, "<statement>")) {
        whileStatement.addChild(ASTChildRole::Body, buildStatement(*statement));
    }
    return whileStatement;
}

ASTNode ASTBuilder::buildRepeatStatement(const ParseNode &node) const {
    ASTNode repeat(ASTNodeKind::RepeatStatement);
    if (const ParseNode *body = firstChildWithLabel(node, "<statement-list>")) {
        repeat.addChild(ASTChildRole::Body, buildStatementList(*body));
    }
    if (const ParseNode *condition = firstChildWithLabel(node, "<expression>")) {
        repeat.addChild(ASTChildRole::Condition, buildExpression(*condition));
    }
    return repeat;
}

ASTNode ASTBuilder::buildForStatement(const ParseNode &node) const {
    ASTNode forStatement(ASTNodeKind::ForStatement);
    bool startFilled = false;
    bool controlVariableFilled = false;

    for (const ParseNode &child : node.getChildren()) {
        if (isTerminal(child, Tokenizer::TOKEN_IDENT) && !controlVariableFilled) {
            forStatement.addChild(ASTChildRole::Variable, makeIdentifier(terminalValue(child)));
            controlVariableFilled = true;
        } else if (isTerminal(child, Tokenizer::TOKEN_TO)) {
            forStatement.setAttribute("direction", "to");
        } else if (isTerminal(child, Tokenizer::TOKEN_DOWNTO)) {
            forStatement.setAttribute("direction", "downto");
        } else if (isNonTerminalLabel(child, "<expression>")) {
            if (!startFilled) {
                forStatement.addChild(ASTChildRole::Start, buildExpression(child));
                startFilled = true;
            } else {
                forStatement.addChild(ASTChildRole::End, buildExpression(child));
            }
        } else if (isNonTerminalLabel(child, "<compound-statement>")) {
            forStatement.addChild(ASTChildRole::Body, buildCompoundStatement(child));
        } else if (isNonTerminalLabel(child, "<statement>")) {
            forStatement.addChild(ASTChildRole::Body, buildStatement(child));
        }
    }
    return forStatement;
}

ASTNode ASTBuilder::buildProcedureOrFunctionCall(const ParseNode &node, bool asFunction) const {
    ASTNode call(asFunction ? ASTNodeKind::FunctionCall : ASTNodeKind::ProcedureCall);
    for (const ParseNode &child : node.getChildren()) {
        if (isTerminal(child, Tokenizer::TOKEN_IDENT)) {
            call.setAttribute("name", terminalValue(child));
            break;
        }
    }
    if (const ParseNode *parameters = firstChildWithLabel(node, "<parameter-list>")) {
        call.addChild(ASTChildRole::Arg, buildParameterList(*parameters));
    }
    return call;
}

ASTNode ASTBuilder::buildParameterList(const ParseNode &node) const {
    ASTNode arguments(ASTNodeKind::Arguments);
    for (const ParseNode &child : node.getChildren()) {
        if (isNonTerminalLabel(child, "<expression>")) {
            arguments.addChild(ASTChildRole::Arg, buildExpression(child));
        }
    }
    return arguments;
}

ASTNode ASTBuilder::buildExpression(const ParseNode &node) const {
    ASTNode current(ASTNodeKind::Unknown);
    bool hasCurrent = false;
    std::string pendingOperator;

    for (const ParseNode &child : node.getChildren()) {
        if (isNonTerminalLabel(child, "<simple-expression>")) {
            ASTNode operand = buildSimpleExpression(child);
            if (!hasCurrent) {
                current = std::move(operand);
                hasCurrent = true;
            } else {
                ASTNode operation(ASTNodeKind::BinaryOperation);
                operation.setAttribute("operator", pendingOperator);
                operation.addChild(ASTChildRole::Left, std::move(current));
                operation.addChild(ASTChildRole::Right, std::move(operand));
                current = std::move(operation);
            }
        } else if (isNonTerminalLabel(child, "<relational-operator>")) {
            pendingOperator = operatorText(child);
        }
    }
    return current;
}

ASTNode ASTBuilder::buildSimpleExpression(const ParseNode &node) const {
    ASTNode current(ASTNodeKind::Unknown);
    bool hasCurrent = false;
    std::string pendingOperator;
    std::string unaryOperator;

    for (const ParseNode &child : node.getChildren()) {
        if (isTerminal(child, Tokenizer::TOKEN_PLUS) || isTerminal(child, Tokenizer::TOKEN_MINUS)) {
            if (!hasCurrent) unaryOperator = operatorText(child);
            else pendingOperator = operatorText(child);
        } else if (isNonTerminalLabel(child, "<additive-operator>")) {
            pendingOperator = operatorText(child);
        } else if (isNonTerminalLabel(child, "<term>")) {
            ASTNode operand = buildTerm(child);
            if (!hasCurrent) {
                if (!unaryOperator.empty()) {
                    ASTNode unary(ASTNodeKind::UnaryOperation);
                    unary.setAttribute("operator", unaryOperator);
                    unary.addChild(ASTChildRole::Expression, std::move(operand));
                    current = std::move(unary);
                } else {
                    current = std::move(operand);
                }
                hasCurrent = true;
            } else {
                ASTNode operation(ASTNodeKind::BinaryOperation);
                operation.setAttribute("operator", pendingOperator);
                operation.addChild(ASTChildRole::Left, std::move(current));
                operation.addChild(ASTChildRole::Right, std::move(operand));
                current = std::move(operation);
            }
        }
    }
    return current;
}

ASTNode ASTBuilder::buildTerm(const ParseNode &node) const {
    ASTNode current(ASTNodeKind::Unknown);
    bool hasCurrent = false;
    std::string pendingOperator;

    for (const ParseNode &child : node.getChildren()) {
        if (isNonTerminalLabel(child, "<factor>")) {
            ASTNode operand = buildFactor(child);
            if (!hasCurrent) {
                current = std::move(operand);
                hasCurrent = true;
            } else {
                ASTNode operation(ASTNodeKind::BinaryOperation);
                operation.setAttribute("operator", pendingOperator);
                operation.addChild(ASTChildRole::Left, std::move(current));
                operation.addChild(ASTChildRole::Right, std::move(operand));
                current = std::move(operation);
            }
        } else if (isNonTerminalLabel(child, "<multiplicative-operator>")) {
            pendingOperator = operatorText(child);
        }
    }
    return current;
}

ASTNode ASTBuilder::buildFactor(const ParseNode &node) const {
    const auto &children = node.getChildren();
    if (children.empty()) return ASTNode(ASTNodeKind::Unknown);

    if (children.size() == 1) {
        const ParseNode &child = children.front();
        if (isNonTerminalLabel(child, "<procedure/function-call>")) return buildProcedureOrFunctionCall(child, true);
        if (isNonTerminalLabel(child, "<variable>")) return buildVariable(child);
        if (child.getSymbol().isTerminal()) return terminalAsLiteralOrIdentifier(child);
    }

    for (std::size_t i = 0; i < children.size(); ++i) {
        if (isTerminal(children[i], Tokenizer::TOKEN_LEFT_PARENTHESES)) {
            for (const ParseNode &child : children) {
                if (isNonTerminalLabel(child, "<expression>")) return buildExpression(child);
            }
        }
        if (isTerminal(children[i], Tokenizer::TOKEN_NOT)) {
            ASTNode unary(ASTNodeKind::UnaryOperation);
            unary.setAttribute("operator", "not");
            for (const ParseNode &child : children) {
                if (isNonTerminalLabel(child, "<factor>")) {
                    unary.addChild(ASTChildRole::Expression, buildFactor(child));
                    break;
                }
            }
            return unary;
        }
    }
    return ASTNode(ASTNodeKind::Unknown);
}

ASTNode ASTBuilder::buildVariable(const ParseNode &node) const {
    ASTNode current(ASTNodeKind::Unknown);
    bool hasBase = false;

    for (const ParseNode &child : node.getChildren()) {
        if (isTerminal(child, Tokenizer::TOKEN_IDENT) && !hasBase) {
            current = ASTNode(ASTNodeKind::Variable);
            current.setAttribute("name", terminalValue(child));
            hasBase = true;
        } else if (isNonTerminalLabel(child, "<component-variable>")) {
            const ParseNode *indexList = firstChildWithLabel(child, "<index-list>");
            if (indexList != nullptr) {
                ASTNode access(ASTNodeKind::ArrayAccess);
                access.addChild(ASTChildRole::Base, std::move(current));
                for (ASTNode &index : buildIndexList(*indexList)) {
                    access.addChild(ASTChildRole::Index, std::move(index));
                }
                current = std::move(access);
            } else {
                for (const ParseNode &componentChild : child.getChildren()) {
                    if (isTerminal(componentChild, Tokenizer::TOKEN_IDENT)) {
                        ASTNode access(ASTNodeKind::FieldAccess);
                        access.setAttribute("field", terminalValue(componentChild));
                        access.addChild(ASTChildRole::Base, std::move(current));
                        current = std::move(access);
                        break;
                    }
                }
            }
        }
    }
    return current;
}

std::vector<ASTNode> ASTBuilder::buildIndexList(const ParseNode &node) const {
    std::vector<ASTNode> indexes;
    for (const ParseNode &child : node.getChildren()) {
        if (child.getSymbol().isTerminal()) {
            if (isTerminal(child, Tokenizer::TOKEN_INT) ||
                isTerminal(child, Tokenizer::TOKEN_IDENT) ||
                isTerminal(child, Tokenizer::TOKEN_CHAR_END) ||
                isTerminal(child, Tokenizer::TOKEN_CHAR_ESCAPE_OR_END)) {
                indexes.push_back(terminalAsLiteralOrIdentifier(child));
            }
        } else if (isNonTerminalLabel(child, "<index-list>")) {
            std::vector<ASTNode> nested = buildIndexList(child);
            indexes.insert(indexes.end(), std::make_move_iterator(nested.begin()), std::make_move_iterator(nested.end()));
        }
    }
    return indexes;
}

ASTNode ASTBuilder::buildConstant(const ParseNode &node) const {
    std::string unaryOperator;
    for (const ParseNode &child : node.getChildren()) {
        if (isTerminal(child, Tokenizer::TOKEN_PLUS) || isTerminal(child, Tokenizer::TOKEN_MINUS)) {
            unaryOperator = operatorText(child);
            continue;
        }
        if (child.getSymbol().isTerminal()) {
            ASTNode value = terminalAsLiteralOrIdentifier(child);
            if (!unaryOperator.empty()) {
                ASTNode unary(ASTNodeKind::UnaryOperation);
                unary.setAttribute("operator", unaryOperator);
                unary.addChild(ASTChildRole::Expression, std::move(value));
                return unary;
            }
            return value;
        }
    }
    return ASTNode(ASTNodeKind::Unknown);
}

std::vector<std::string> ASTBuilder::collectIdentifierList(const ParseNode &node) const {
    std::vector<std::string> names;
    for (const ParseNode &child : node.getChildren()) {
        if (isTerminal(child, Tokenizer::TOKEN_IDENT)) {
            names.push_back(terminalValue(child));
        }
    }
    return names;
}

const ParseNode *ASTBuilder::firstChildWithLabel(const ParseNode &node, const std::string &label) const {
    for (const ParseNode &child : node.getChildren()) {
        if (hasLabel(child, label)) return &child;
    }
    return nullptr;
}

std::string ASTBuilder::terminalValue(const ParseNode &node) const {
    std::string value = node.getSymbol().getValue();
    return value.empty() ? node.getLabel() : value;
}

std::string ASTBuilder::operatorText(const ParseNode &node) const {
    if (node.getSymbol().isTerminal()) {
        std::string value = terminalValue(node);
        return value.empty() ? node.getLabel() : value;
    }

    for (const ParseNode &child : node.getChildren()) {
        if (child.getSymbol().isTerminal()) return operatorText(child);
    }
    return node.getLabel();
}

ASTNode ASTBuilder::terminalAsLiteralOrIdentifier(const ParseNode &node) const {
    const int id = node.getSymbol().getId();
    const std::string value = terminalValue(node);

    if (id == Tokenizer::TOKEN_INT) return ASTNode(ASTNodeKind::IntegerLiteral).setAttribute("value", value);
    if (id == Tokenizer::TOKEN_REAL) return ASTNode(ASTNodeKind::RealLiteral).setAttribute("value", value);
    if (id == Tokenizer::TOKEN_CHAR_END || id == Tokenizer::TOKEN_CHAR_ESCAPE_OR_END) {
        return ASTNode(ASTNodeKind::CharLiteral).setAttribute("value", value);
    }
    if (id == Tokenizer::TOKEN_STRING_ESCAPE_OR_END) {
        return ASTNode(ASTNodeKind::StringLiteral).setAttribute("value", value);
    }
    if (id == Tokenizer::TOKEN_IDENT) {
        const std::string lowered = lowerCopy(value);
        if (lowered == "true" || lowered == "false") {
            return ASTNode(ASTNodeKind::BooleanLiteral).setAttribute("value", lowered);
        }
        return ASTNode(ASTNodeKind::Variable).setAttribute("name", value);
    }

    ASTNode unknown(ASTNodeKind::Unknown);
    unknown.setAttribute("token", node.getLabel());
    return unknown;
}

ASTNode ASTBuilder::makeIdentifier(const std::string &name) const {
    ASTNode identifier(ASTNodeKind::Identifier);
    identifier.setAttribute("name", name);
    return identifier;
}

bool ASTBuilder::hasLabel(const ParseNode &node, const std::string &label) const {
    return node.getLabel() == label;
}

bool ASTBuilder::isTerminal(const ParseNode &node, int tokenType) const {
    return node.getSymbol().isTerminal() && node.getSymbol().getId() == tokenType;
}