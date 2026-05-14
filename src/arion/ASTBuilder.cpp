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