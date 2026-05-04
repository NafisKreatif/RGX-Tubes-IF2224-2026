#include "Parser.hpp"
#include <sstream>
#include <utility>

using namespace arion;

ParseNode::ParseNode(std::string label) : label_(std::move(label)) {}

ParseNode &ParseNode::addChild(ParseNode child) {
    children_.push_back(std::move(child));
    return children_.back();
}

const std::string &ParseNode::getLabel() const {
    return label_;
}

const std::vector<ParseNode> &ParseNode::getChildren() const {
    return children_;
}

std::string ParseNode::toString(int depth) const {
    std::ostringstream out;
    out << std::string(depth * 4, ' ') << label_ << '\n';
    for (const ParseNode &child : children_) {
        out << child.toString(depth + 1);
    }
    return out.str();
}

ParserError::ParserError(const std::string &message) : std::runtime_error(message) {}

Parser::Parser(std::vector<Token> tokens) {
    setTokens(std::move(tokens));
}

void Parser::setTokens(std::vector<Token> tokens) {
    tokens_ = std::move(tokens);
    current_ = 0;
    skipIgnoredTokens();
}

ParseNode Parser::parse() {
    ParseNode root = parseProgram();
    expect(Tokenizer::TOKEN_EOF);
    return root;
}

const Token &Parser::peek() const {
    if (current_ >= tokens_.size()) {
        return eofToken_;
    }
    return tokens_[current_];
}

const Token &Parser::peekNext() const {
    std::size_t next = current_ + 1;
    while (next < tokens_.size() && (tokens_[next].type == Tokenizer::TOKEN_COMMENT_CURLY_END || tokens_[next].type == Tokenizer::TOKEN_COMMENT_PARENTHESES_END)) {
        ++next;
    }
    if (next >= tokens_.size()) {
        return eofToken_;
    }
    return tokens_[next];
}

const Token &Parser::previous() const {
    if (current_ == 0 || current_ - 1 >= tokens_.size()) {
        return eofToken_;
    }
    return tokens_[current_ - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == Tokenizer::TOKEN_EOF;
}

bool Parser::check(int tokenType) const {
    return peek().type == tokenType;
}

bool Parser::match(int tokenType) {
    if (!check(tokenType)) {
        return false;
    }
    advance();
    return true;
}

Token Parser::advance() {
    if (!isAtEnd() && current_ < tokens_.size()) {
        Token consumed = tokens_[current_];
        ++current_;
        skipIgnoredTokens();
        return consumed;
    }
    return previous();
}

Token Parser::expect(int tokenType) {
    if (check(tokenType)) {
        return advance();
    }
    syntaxError(terminalName(tokenType));
    return eofToken_;
}

ParseNode Parser::variableNode(NonTerminal variable) const {
    return ParseNode(variableName(variable));
}

ParseNode Parser::terminalNode(const Token &token) const {
    return ParseNode(tokenName(token));
}

std::string Parser::variableName(NonTerminal variable) const {
    switch (variable) {
        case PROGRAM:
            return "<program>";
        case PROGRAM_HEADER:
            return "<program-header>";
        case DECLARATION_PART:
            return "<declaration-part>";
        case CONST_DECLARATION:
            return "<const-declaration>";
        case CONSTANT:
            return "<constant>";
        case TYPE_DECLARATION:
            return "<type-declaration>";
        case VAR_DECLARATION:
            return "<var-declaration>";
        case IDENTIFIER_LIST:
            return "<identifier-list>";
        case TYPE:
            return "<type>";
        case ARRAY_TYPE:
            return "<array-type>";
        case RANGE:
            return "<range>";
        case ENUMERATED:
            return "<enumerated>";
        case RECORD_TYPE:
            return "<record-type>";
        case FIELD_LIST:
            return "<field-list>";
        case FIELD_PART:
            return "<field-part>";
        case SUBPROGRAM_DECLARATION:
            return "<subprogram-declaration>";
        case PROCEDURE_DECLARATION:
            return "<procedure-declaration>";
        case FUNCTION_DECLARATION:
            return "<function-declaration>";
        case BLOCK:
            return "<block>";
        case FORMAL_PARAMETER_LIST:
            return "<formal-parameter-list>";
        case PARAMETER_GROUP:
            return "<parameter-group>";
        case COMPOUND_STATEMENT:
            return "<compound-statement>";
        case STATEMENT_LIST:
            return "<statement-list>";
        case STATEMENT:
            return "<statement>";
        case ASSIGNMENT_STATEMENT:
            return "<assignment-statement>";
        case IF_STATEMENT:
            return "<if-statement>";
        case CASE_STATEMENT:
            return "<case-statement>";
        case CASE_BLOCK:
            return "<case-block>";
        case WHILE_STATEMENT:
            return "<while-statement>";
        case REPEAT_STATEMENT:
            return "<repeat-statement>";
        case FOR_STATEMENT:
            return "<for-statement>";
        case PROCEDURE_FUNCTION_CALL:
            return "<procedure/function-call>";
        case PARAMETER_LIST:
            return "<parameter-list>";
        case EXPRESSION:
            return "<expression>";
        case SIMPLE_EXPRESSION:
            return "<simple-expression>";
        case TERM:
            return "<term>";
        case FACTOR:
            return "<factor>";
        case VARIABLE:
            return "<variable>";
        case COMPONENT_VARIABLE:
            return "<component-variable>";
        case INDEX_LIST:
            return "<index-list>";
        case RELATIONAL_OPERATOR:
            return "<relational-operator>";
        case ADDITIVE_OPERATOR:
            return "<additive-operator>";
        case MULTIPLICATIVE_OPERATOR:
            return "<multiplicative-operator>";
    }
    return "<unknown>";
}

std::string Parser::terminalName(int tokenType) const {
    return Tokenizer::tokenToString(Token{tokenType, ""});
}

std::string Parser::tokenName(const Token &token) const {
    return Tokenizer::tokenToString(token);
}

void Parser::syntaxError(const std::string &expectedName) const {
    std::ostringstream message;
    message << "Syntax error: unexpected token " << tokenName(peek())
            << ", expected " << expectedName;
    throw ParserError(message.str());
}

void Parser::skipIgnoredTokens() {
    while (current_ < tokens_.size() && (tokens_[current_].type == Tokenizer::TOKEN_COMMENT_CURLY_END || tokens_[current_].type == Tokenizer::TOKEN_COMMENT_PARENTHESES_END)) {
        ++current_;
    }
}

ParseNode Parser::parseProgram() {
    ParseNode node = variableNode(PROGRAM);
    node.addChild(parseProgramHeader());
    node.addChild(parseDeclarationPart());
    node.addChild(parseCompoundStatement());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_PERIOD)));
    return node;
}

ParseNode Parser::parseProgramHeader() {
    ParseNode node = variableNode(PROGRAM_HEADER);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_PROGRAM)));
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_SEMICOLON)));
    return node;
}

ParseNode Parser::parseDeclarationPart() {
    ParseNode node = variableNode(DECLARATION_PART);

    while (check(Tokenizer::TOKEN_CONST)) {
        node.addChild(parseConstDeclaration());
    }
    while (check(Tokenizer::TOKEN_TYPE)) {
        node.addChild(parseTypeDeclaration());
    }
    while (check(Tokenizer::TOKEN_VAR)) {
        node.addChild(parseVarDeclaration());
    }
    while (check(Tokenizer::TOKEN_PROCEDURE) || check(Tokenizer::TOKEN_FUNCTION)) {
        node.addChild(parseSubprogramDeclaration());
    }

    return node;
}

ParseNode Parser::parseConstDeclaration() {
    throw ParserError("parseConstDeclaration is not implemented yet");
}

ParseNode Parser::parseConstant() {
    throw ParserError("parseConstant is not implemented yet");
}

ParseNode Parser::parseTypeDeclaration() {
    throw ParserError("parseTypeDeclaration is not implemented yet");
}

ParseNode Parser::parseVarDeclaration() {
    throw ParserError("parseVarDeclaration is not implemented yet");
}

ParseNode Parser::parseIdentifierList() {
    throw ParserError("parseIdentifierList is not implemented yet");
}

ParseNode Parser::parseType() {
    throw ParserError("parseType is not implemented yet");
}

ParseNode Parser::parseArrayType() {
    throw ParserError("parseArrayType is not implemented yet");
}

ParseNode Parser::parseRange() {
    throw ParserError("parseRange is not implemented yet");
}

ParseNode Parser::parseEnumerated() {
    throw ParserError("parseEnumerated is not implemented yet");
}

ParseNode Parser::parseRecordType() {
    throw ParserError("parseRecordType is not implemented yet");
}

ParseNode Parser::parseFieldList() {
    throw ParserError("parseFieldList is not implemented yet");
}

ParseNode Parser::parseFieldPart() {
    throw ParserError("parseFieldPart is not implemented yet");
}

ParseNode Parser::parseSubprogramDeclaration() {
    throw ParserError("parseSubprogramDeclaration is not implemented yet");
}

ParseNode Parser::parseProcedureDeclaration() {
    throw ParserError("parseProcedureDeclaration is not implemented yet");
}

ParseNode Parser::parseFunctionDeclaration() {
    throw ParserError("parseFunctionDeclaration is not implemented yet");
}

ParseNode Parser::parseBlock() {
    throw ParserError("parseBlock is not implemented yet");
}

ParseNode Parser::parseFormalParameterList() {
    throw ParserError("parseFormalParameterList is not implemented yet");
}

ParseNode Parser::parseParameterGroup() {
    throw ParserError("parseParameterGroup is not implemented yet");
}

ParseNode Parser::parseCompoundStatement() {
    throw ParserError("parseCompoundStatement is not implemented yet");
}

ParseNode Parser::parseStatementList() {
    throw ParserError("parseStatementList is not implemented yet");
}

ParseNode Parser::parseStatement() {
    throw ParserError("parseStatement is not implemented yet");
}

ParseNode Parser::parseAssignmentStatement() {
    throw ParserError("parseAssignmentStatement is not implemented yet");
}

ParseNode Parser::parseIfStatement() {
    throw ParserError("parseIfStatement is not implemented yet");
}

ParseNode Parser::parseCaseStatement() {
    throw ParserError("parseCaseStatement is not implemented yet");
}

ParseNode Parser::parseCaseBlock() {
    throw ParserError("parseCaseBlock is not implemented yet");
}

ParseNode Parser::parseWhileStatement() {
    throw ParserError("parseWhileStatement is not implemented yet");
}

ParseNode Parser::parseRepeatStatement() {
    throw ParserError("parseRepeatStatement is not implemented yet");
}

ParseNode Parser::parseForStatement() {
    throw ParserError("parseForStatement is not implemented yet");
}

ParseNode Parser::parseProcedureOrFunctionCall() {
    throw ParserError("parseProcedureOrFunctionCall is not implemented yet");
}

ParseNode Parser::parseParameterList() {

    throw ParserError("parseParameterList is not implemented yet");
}

ParseNode Parser::parseExpression() {

    throw ParserError("parseExpression is not implemented yet");
}

ParseNode Parser::parseSimpleExpression() {

    throw ParserError("parseSimpleExpression is not implemented yet");
}

ParseNode Parser::parseTerm() {
    throw ParserError("parseTerm is not implemented yet");
}

ParseNode Parser::parseFactor() {
    throw ParserError("parseFactor is not implemented yet");
}

ParseNode Parser::parseVariable() {
    throw ParserError("parseVariable is not implemented yet");
}

ParseNode Parser::parseComponentVariable() {
    throw ParserError("parseComponentVariable is not implemented yet");
}

ParseNode Parser::parseIndexList() {
    throw ParserError("parseIndexList is not implemented yet");
}

ParseNode Parser::parseRelationalOperator() {
    throw ParserError("parseRelationalOperator is not implemented yet");
}

ParseNode Parser::parseAdditiveOperator() {
    throw ParserError("parseAdditiveOperator is not implemented yet");
}

ParseNode Parser::parseMultiplicativeOperator() {
    throw ParserError("parseMultiplicativeOperator is not implemented yet");
}
