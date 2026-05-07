#include "Parser.hpp"
#include <sstream>
#include <utility>

using namespace arion;

Symbol::Symbol(int id, std::string label, bool isTerminal, std::string value) {
    this->id_ = id;
    this->label_ = std::move(label);
    this->isTerminal_ = isTerminal;
    this->value_ = std::move(value);
}
std::string Symbol::toString() const {
    return label_;
}
bool Symbol::isTerminal() const {
    return isTerminal_;
}

ParseNode::ParseNode(Symbol symbol) : symbol_(std::move(symbol)) {}

ParseNode &ParseNode::addChild(ParseNode child) {
    children_.push_back(std::move(child));
    return children_.back();
}

Symbol ParseNode::getSymbol() const {
    return symbol_;
}

std::string ParseNode::getLabel() const {
    return symbol_.toString();
}

const std::vector<ParseNode> &ParseNode::getChildren() const {
    return children_;
}

std::string ParseNode::toString() const {
    std::vector<bool> isLast;
    return toStringHelper(0, isLast);
}

// Mencetak parse tree dengan prefix cabang sesuai posisi anak pada level yang sama.
std::string ParseNode::toStringHelper(int depth, std::vector<bool> &isLast) const {
    std::ostringstream out;
    for (int i = 0; i < depth; i++) {
        out << ((i == depth - 1)
                    ? (isLast[i] ? "└── " : "├── ")
                    : (isLast[i] ? "    " : "│   "));
    }
    out << symbol_.toString() << '\n';
    for (int i = 0; i < (int)children_.size(); i++) {
        isLast.push_back(i == ((int)children_.size() - 1));
        out << children_[i].toStringHelper(depth + 1, isLast);
        isLast.pop_back();
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
    return lookahead(0);
}

const Token &Parser::peekNext() const {
    return lookahead(1);
}

const Token &Parser::lookahead(std::size_t offset) const {
    std::size_t index = current_;
    std::size_t skipped = 0;

    // Token komentar tetap ada dari lexer, tetapi tidak dihitung sebagai lookahead parser.
    while (index < tokens_.size()) {
        if (tokens_[index].type != Tokenizer::TOKEN_COMMENT_CURLY_END &&
            tokens_[index].type != Tokenizer::TOKEN_COMMENT_PARENTHESES_END) {
            if (skipped == offset) {
                return tokens_[index];
            }
            ++skipped;
        }
        ++index;
    }

    return eofToken_;
}

bool Parser::isAtEnd() const {
    return peek().type == Tokenizer::TOKEN_EOF;
}

bool Parser::check(int tokenType) const {
    return peek().type == tokenType;
}

bool Parser::checkNext(int tokenType) const {
    return peekNext().type == tokenType;
}

Token Parser::advance() {
    if (!isAtEnd() && current_ < tokens_.size()) {
        Token consumed = tokens_[current_];
        ++current_;
        skipIgnoredTokens();
        return consumed;
    }
    return eofToken_;
}

Token Parser::expect(int tokenType) {
    // Semua token wajib dikonsumsi lewat expect supaya pesan syntax error konsisten.
    if (check(tokenType)) {
        return advance();
    }
    syntaxError(terminalName(tokenType));
    return eofToken_;
}

ParseNode Parser::variableNode(NonTerminal variable) const {
    return ParseNode(Symbol{variable, variableName(variable)});
}

ParseNode Parser::terminalNode(const Token &token) const {
    return ParseNode(Symbol{token.type, tokenName(token), true, token.value});
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
    // Setelah advance, posisi parser dilompati melewati token komentar.
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

    // Urutan deklarasi mengikuti grammar: const, type, var, lalu procedure/function.
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
    ParseNode node = variableNode(CONST_DECLARATION);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_CONST)));
    do {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_EQUAL_END)));
        node.addChild(parseConstant());
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_SEMICOLON)));
    } while (check(Tokenizer::TOKEN_IDENT));

    return node;
}

ParseNode Parser::parseConstant() {
    ParseNode node = variableNode(CONSTANT);

    // Char dan string sudah lengkap sebagai satu token; angka/ident boleh diawali tanda.
    if (check(Tokenizer::TOKEN_CHAR_END)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_CHAR_END)));
        return node;
    }
    if (check(Tokenizer::TOKEN_STRING_ESCAPE_OR_END)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_STRING_ESCAPE_OR_END)));
        return node;
    }
    if (check(Tokenizer::TOKEN_CHAR_ESCAPE_OR_END)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_CHAR_ESCAPE_OR_END)));
        return node;
    }

    if (check(Tokenizer::TOKEN_PLUS) || check(Tokenizer::TOKEN_MINUS)) {
        node.addChild(terminalNode(advance()));
    }

    if (check(Tokenizer::TOKEN_IDENT) || check(Tokenizer::TOKEN_INT) || check(Tokenizer::TOKEN_REAL)) {
        node.addChild(terminalNode(advance()));
        return node;
    }

    syntaxError("constant");
    return node;
}

ParseNode Parser::parseTypeDeclaration() {
    ParseNode node = variableNode(TYPE_DECLARATION);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_TYPE)));
    do {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_EQUAL_END)));
        node.addChild(parseType());
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_SEMICOLON)));
    } while (check(Tokenizer::TOKEN_IDENT));

    return node;
}

ParseNode Parser::parseVarDeclaration() {
    ParseNode node = variableNode(VAR_DECLARATION);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_VAR)));
    do {
        node.addChild(parseIdentifierList());
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_COLON)));
        node.addChild(parseType());
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_SEMICOLON)));
    } while (check(Tokenizer::TOKEN_IDENT));

    return node;
}

ParseNode Parser::parseIdentifierList() {
    ParseNode node = variableNode(IDENTIFIER_LIST);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    while (check(Tokenizer::TOKEN_COMMA)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_COMMA)));
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    }

    return node;
}

ParseNode Parser::parseType() {
    ParseNode node = variableNode(TYPE);

    // Alternatif type dipilih dari token pertama setiap produksi.
    if (check(Tokenizer::TOKEN_ARRAY)) {
        node.addChild(parseArrayType());
        return node;
    }
    if (check(Tokenizer::TOKEN_LEFT_PARENTHESES)) {
        node.addChild(parseEnumerated());
        return node;
    }
    if (check(Tokenizer::TOKEN_RECORD)) {
        node.addChild(parseRecordType());
        return node;
    }
    if (check(Tokenizer::TOKEN_PLUS) ||
        check(Tokenizer::TOKEN_MINUS) ||
        check(Tokenizer::TOKEN_INT) ||
        check(Tokenizer::TOKEN_REAL) ||
        check(Tokenizer::TOKEN_CHAR_END) ||
        check(Tokenizer::TOKEN_CHAR_ESCAPE_OR_END) ||
        check(Tokenizer::TOKEN_STRING_ESCAPE_OR_END)) {
        node.addChild(parseRange());
        return node;
    }
    if (check(Tokenizer::TOKEN_IDENT)) {
        if (checkNext(Tokenizer::TOKEN_PERIOD)) {
            node.addChild(parseRange());
        } else {
            node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
        }
        return node;
    }

    syntaxError("type");
    return node;
}

ParseNode Parser::parseArrayType() {
    ParseNode node = variableNode(ARRAY_TYPE);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_ARRAY)));
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_LEFT_BRACKET)));
    if (check(Tokenizer::TOKEN_IDENT) && !checkNext(Tokenizer::TOKEN_PERIOD)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    } else {
        node.addChild(parseRange());
    }
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_RIGHT_BRACKET)));
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_OF)));
    node.addChild(parseType());

    return node;
}

ParseNode Parser::parseRange() {
    ParseNode node = variableNode(RANGE);
    node.addChild(parseConstant());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_PERIOD)));
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_PERIOD)));
    node.addChild(parseConstant());

    return node;
}

ParseNode Parser::parseEnumerated() {
    ParseNode node = variableNode(ENUMERATED);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_LEFT_PARENTHESES)));
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    while (check(Tokenizer::TOKEN_COMMA)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_COMMA)));
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    }
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_RIGHT_PARENTHESES)));

    return node;
}

ParseNode Parser::parseRecordType() {
    ParseNode node = variableNode(RECORD_TYPE);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_RECORD)));
    node.addChild(parseFieldList());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_END)));

    return node;
}

ParseNode Parser::parseFieldList() {
    ParseNode node = variableNode(FIELD_LIST);
    node.addChild(parseFieldPart());
    while (check(Tokenizer::TOKEN_SEMICOLON)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_SEMICOLON)));
        // Semicolon terakhir sebelum end pada record tidak memulai field baru.
        if (check(Tokenizer::TOKEN_END)) {
            break;
        }
        node.addChild(parseFieldPart());
    }

    return node;
}

ParseNode Parser::parseFieldPart() {
    ParseNode node = variableNode(FIELD_PART);
    node.addChild(parseIdentifierList());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_COLON)));
    node.addChild(parseType());

    return node;
}

ParseNode Parser::parseSubprogramDeclaration() {
    ParseNode node = variableNode(SUBPROGRAM_DECLARATION);
    if (check(Tokenizer::TOKEN_PROCEDURE)) {
        node.addChild(parseProcedureDeclaration());
        return node;
    }
    if (check(Tokenizer::TOKEN_FUNCTION)) {
        node.addChild(parseFunctionDeclaration());
        return node;
    }
    syntaxError("subprogram-declaration");
    return node;
}

ParseNode Parser::parseProcedureDeclaration() {
    ParseNode node = variableNode(PROCEDURE_DECLARATION);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_PROCEDURE)));
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    if (check(Tokenizer::TOKEN_LEFT_PARENTHESES)) {
        node.addChild(parseFormalParameterList());
    }
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_SEMICOLON)));
    node.addChild(parseBlock());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_SEMICOLON)));
    return node;
}

ParseNode Parser::parseFunctionDeclaration() {
    ParseNode node = variableNode(FUNCTION_DECLARATION);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_FUNCTION)));
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    if (check(Tokenizer::TOKEN_LEFT_PARENTHESES)) {
        node.addChild(parseFormalParameterList());
    }
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_COLON)));
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_SEMICOLON)));
    node.addChild(parseBlock());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_SEMICOLON)));
    return node;
}

ParseNode Parser::parseBlock() {
    ParseNode node = variableNode(BLOCK);
    node.addChild(parseDeclarationPart());
    node.addChild(parseCompoundStatement());
    return node;
}

ParseNode Parser::parseFormalParameterList() {
    ParseNode node = variableNode(FORMAL_PARAMETER_LIST);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_LEFT_PARENTHESES)));
    node.addChild(parseParameterGroup());
    while (check(Tokenizer::TOKEN_SEMICOLON)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_SEMICOLON)));
        node.addChild(parseParameterGroup());
    }
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_RIGHT_PARENTHESES)));
    return node;
}

ParseNode Parser::parseParameterGroup() {
    ParseNode node = variableNode(PARAMETER_GROUP);
    node.addChild(parseIdentifierList());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_COLON)));
    if (check(Tokenizer::TOKEN_IDENT)) node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    else node.addChild(parseArrayType());

    return node;
}

ParseNode Parser::parseCompoundStatement() {
    ParseNode node = variableNode(COMPOUND_STATEMENT);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_BEGIN)));
    node.addChild(parseStatementList());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_END)));
    return node;
}

ParseNode Parser::parseStatementList() {
    ParseNode node = variableNode(STATEMENT_LIST);
    node.addChild(parseStatement());
    while (check(Tokenizer::TOKEN_SEMICOLON)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_SEMICOLON)));
        if (check(Tokenizer::TOKEN_END) || check(Tokenizer::TOKEN_UNTIL)) break;
        node.addChild(parseStatement());
    }
    return node;
}

ParseNode Parser::parseStatement() {
    ParseNode node = variableNode(STATEMENT);
    // Identifier perlu lookahead karena bisa menjadi assignment atau procedure/function call.
    if (check(Tokenizer::TOKEN_IDENT)) {
        if (checkNext(Tokenizer::TOKEN_LEFT_BRACKET) || checkNext(Tokenizer::TOKEN_PERIOD) ||
            checkNext(Tokenizer::TOKEN_BECOMES)) {
            node.addChild(parseAssignmentStatement());
            return node;
        }
        if (checkNext(Tokenizer::TOKEN_LEFT_PARENTHESES)) {
            node.addChild(parseProcedureOrFunctionCall());
            return node;
        }
    }
    if (check(Tokenizer::TOKEN_IF)) {
        node.addChild(parseIfStatement());
        return node;
    }
    if (check(Tokenizer::TOKEN_CASE)) {
        node.addChild(parseCaseStatement());
        return node;
    }
    if (check(Tokenizer::TOKEN_WHILE)) {
        node.addChild(parseWhileStatement());
        return node;
    }
    if (check(Tokenizer::TOKEN_REPEAT)) {
        node.addChild(parseRepeatStatement());
        return node;
    }
    if (check(Tokenizer::TOKEN_FOR)) {
        node.addChild(parseForStatement());
        return node;
    }
    if (check(Tokenizer::TOKEN_END) || check(Tokenizer::TOKEN_UNTIL) || check(Tokenizer::TOKEN_ELSE) || check(Tokenizer::TOKEN_SEMICOLON)) {
        return node;
    }
    syntaxError("statement");
    return node;
}

ParseNode Parser::parseAssignmentStatement() {
    ParseNode node = variableNode(ASSIGNMENT_STATEMENT);
    node.addChild(parseVariable());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_BECOMES)));
    node.addChild(parseExpression());
    return node;
}

ParseNode Parser::parseIfStatement() {
    ParseNode node = variableNode(IF_STATEMENT);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_IF)));
    node.addChild(parseExpression());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_THEN)));
    node.addChild(parseStatement());
    if (check(Tokenizer::TOKEN_ELSE)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_ELSE)));
        node.addChild(parseStatement());
    }
    return node;
}

ParseNode Parser::parseCaseStatement() {
    ParseNode node = variableNode(CASE_STATEMENT);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_CASE)));
    node.addChild(parseExpression());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_OF)));
    node.addChild(parseCaseBlock());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_END)));
    return node;
}

ParseNode Parser::parseCaseBlock() {
    ParseNode node = variableNode(CASE_BLOCK);
    node.addChild(parseConstant());
    while (check(Tokenizer::TOKEN_COMMA)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_COMMA)));
        node.addChild(parseConstant());
    }
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_COLON)));
    node.addChild(parseStatement());
    while (check(Tokenizer::TOKEN_SEMICOLON)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_SEMICOLON)));
        // Case berikutnya dibaca rekursif sampai token end menutup case statement.
        if (!check(Tokenizer::TOKEN_END)) node.addChild(parseCaseBlock());
    }
    return node;
}

ParseNode Parser::parseWhileStatement() {
    ParseNode node = variableNode(WHILE_STATEMENT);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_WHILE)));
    node.addChild(parseExpression());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_DO)));
    node.addChild(parseStatement());
    return node;
}

ParseNode Parser::parseRepeatStatement() {
    ParseNode node = variableNode(REPEAT_STATEMENT);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_REPEAT)));
    node.addChild(parseStatementList());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_UNTIL)));
    node.addChild(parseExpression());

    return node;
}

ParseNode Parser::parseForStatement() {
    ParseNode node = variableNode(FOR_STATEMENT);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_FOR)));
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_BECOMES)));
    node.addChild(parseExpression());
    if (check(Tokenizer::TOKEN_TO)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_TO)));
    } else if (check(Tokenizer::TOKEN_DOWNTO)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_DOWNTO)));
    } else {
        syntaxError("tosy or downtosy");
    }
    node.addChild(parseExpression());
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_DO)));
    node.addChild(parseStatement());

    return node;
}

ParseNode Parser::parseProcedureOrFunctionCall() {
    ParseNode node = variableNode(PROCEDURE_FUNCTION_CALL);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_LEFT_PARENTHESES)));
    // Parameter opsional hanya diparse jika token berikutnya dapat memulai expression.
    if (check(Tokenizer::TOKEN_PLUS) ||
        check(Tokenizer::TOKEN_MINUS) ||
        check(Tokenizer::TOKEN_IDENT) ||
        check(Tokenizer::TOKEN_INT) ||
        check(Tokenizer::TOKEN_REAL) ||
        check(Tokenizer::TOKEN_CHAR_ESCAPE_OR_END) ||
        check(Tokenizer::TOKEN_CHAR_END) ||
        check(Tokenizer::TOKEN_STRING_ESCAPE_OR_END) ||
        check(Tokenizer::TOKEN_LEFT_PARENTHESES) ||
        check(Tokenizer::TOKEN_NOT)) {
        node.addChild(parseParameterList());
    }
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_RIGHT_PARENTHESES)));

    return node;
}

ParseNode Parser::parseParameterList() {
    ParseNode node = variableNode(PARAMETER_LIST);
    node.addChild(parseExpression());
    while (check(Tokenizer::TOKEN_COMMA)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_COMMA)));
        node.addChild(parseExpression());
    }

    return node;
}

ParseNode Parser::parseExpression() {
    ParseNode node = variableNode(EXPRESSION);
    node.addChild(parseSimpleExpression());
    // Relational operator berada di luar simple-expression agar prioritasnya paling rendah.
    if (check(Tokenizer::TOKEN_EQUAL_END) ||
        check(Tokenizer::TOKEN_NOT_EQUAL) ||
        check(Tokenizer::TOKEN_GREATER_THAN) ||
        check(Tokenizer::TOKEN_GREATER_THAN_OR_EQUAL) ||
        check(Tokenizer::TOKEN_LESS_THAN) ||
        check(Tokenizer::TOKEN_LESS_THAN_OR_EQUAL)) {
        node.addChild(parseRelationalOperator());
        node.addChild(parseSimpleExpression());
    }

    return node;
}

ParseNode Parser::parseSimpleExpression() {
    ParseNode node = variableNode(SIMPLE_EXPRESSION);
    // Unary + atau - hanya valid di awal simple-expression.
    if (check(Tokenizer::TOKEN_PLUS) || check(Tokenizer::TOKEN_MINUS)) {
        node.addChild(terminalNode(advance()));
    }
    node.addChild(parseTerm());
    while (check(Tokenizer::TOKEN_PLUS) ||
           check(Tokenizer::TOKEN_MINUS) ||
           check(Tokenizer::TOKEN_OR)) {
        node.addChild(parseAdditiveOperator());
        node.addChild(parseTerm());
    }

    return node;
}

ParseNode Parser::parseTerm() {
    ParseNode node = variableNode(TERM);
    node.addChild(parseFactor());
    // Operator perkalian, pembagian, modulo, dan and diproses di level term.
    while (check(Tokenizer::TOKEN_TIMES) ||
           check(Tokenizer::TOKEN_RDIV) ||
           check(Tokenizer::TOKEN_IDIV) ||
           check(Tokenizer::TOKEN_MOD) ||
           check(Tokenizer::TOKEN_AND)) {
        node.addChild(parseMultiplicativeOperator());
        node.addChild(parseFactor());
    }

    return node;
}

ParseNode Parser::parseFactor() {
    ParseNode node = variableNode(FACTOR);

    // Factor adalah unit terkecil expression: literal, variable, call, grouping, atau not.
    if (check(Tokenizer::TOKEN_IDENT)) {
        if (checkNext(Tokenizer::TOKEN_LEFT_PARENTHESES)) {
            node.addChild(parseProcedureOrFunctionCall());
        } else if (checkNext(Tokenizer::TOKEN_LEFT_BRACKET) ||
                   checkNext(Tokenizer::TOKEN_PERIOD)) {
            node.addChild(parseVariable());
        } else {
            node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
        }
        return node;
    }
    if (check(Tokenizer::TOKEN_INT) ||
        check(Tokenizer::TOKEN_REAL) ||
        check(Tokenizer::TOKEN_CHAR_END) ||
        check(Tokenizer::TOKEN_CHAR_ESCAPE_OR_END) ||
        check(Tokenizer::TOKEN_STRING_ESCAPE_OR_END)) {
        node.addChild(terminalNode(advance()));
        return node;
    }
    if (check(Tokenizer::TOKEN_LEFT_PARENTHESES)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_LEFT_PARENTHESES)));
        node.addChild(parseExpression());
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_RIGHT_PARENTHESES)));
        return node;
    }
    if (check(Tokenizer::TOKEN_NOT)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_NOT)));
        node.addChild(parseFactor());
        return node;
    }

    syntaxError("factor");
    return node;
}

ParseNode Parser::parseVariable() {
    ParseNode node = variableNode(VARIABLE);
    node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    while (check(Tokenizer::TOKEN_LEFT_BRACKET) ||
           check(Tokenizer::TOKEN_PERIOD)) {
        node.addChild(parseComponentVariable());
    }

    return node;
}

ParseNode Parser::parseComponentVariable() {
    ParseNode node = variableNode(COMPONENT_VARIABLE);
    if (check(Tokenizer::TOKEN_LEFT_BRACKET)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_LEFT_BRACKET)));
        node.addChild(parseIndexList());
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_RIGHT_BRACKET)));
    } else if (check(Tokenizer::TOKEN_PERIOD)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_PERIOD)));
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_IDENT)));
    } else {
        syntaxError("component variable");
    }

    return node;
}

ParseNode Parser::parseIndexList() {
    ParseNode node = variableNode(INDEX_LIST);
    if (check(Tokenizer::TOKEN_INT) ||
        check(Tokenizer::TOKEN_IDENT) ||
        check(Tokenizer::TOKEN_CHAR_END) ||
        check(Tokenizer::TOKEN_CHAR_ESCAPE_OR_END)) {
        node.addChild(terminalNode(advance()));
    } else {
        syntaxError("index element");
    }
    // Sesuai grammar, elemen setelah comma direpresentasikan sebagai index-list baru.
    while (check(Tokenizer::TOKEN_COMMA)) {
        node.addChild(terminalNode(expect(Tokenizer::TOKEN_COMMA)));
        node.addChild(parseIndexList());
    }

    return node;
}

ParseNode Parser::parseRelationalOperator() {
    ParseNode node = variableNode(RELATIONAL_OPERATOR);
    if (check(Tokenizer::TOKEN_EQUAL_END) ||
        check(Tokenizer::TOKEN_NOT_EQUAL) ||
        check(Tokenizer::TOKEN_GREATER_THAN) ||
        check(Tokenizer::TOKEN_GREATER_THAN_OR_EQUAL) ||
        check(Tokenizer::TOKEN_LESS_THAN) ||
        check(Tokenizer::TOKEN_LESS_THAN_OR_EQUAL)) {
        node.addChild(terminalNode(advance()));
    } else {
        syntaxError("relational operator");
    }

    return node;
}

ParseNode Parser::parseAdditiveOperator() {
    ParseNode node = variableNode(ADDITIVE_OPERATOR);
    if (check(Tokenizer::TOKEN_PLUS) ||
        check(Tokenizer::TOKEN_MINUS) ||
        check(Tokenizer::TOKEN_OR)) {
        node.addChild(terminalNode(advance()));
    } else {
        syntaxError("additive operator");
    }

    return node;
}

ParseNode Parser::parseMultiplicativeOperator() {
    ParseNode node = variableNode(MULTIPLICATIVE_OPERATOR);
    if (check(Tokenizer::TOKEN_TIMES) ||
        check(Tokenizer::TOKEN_RDIV) ||
        check(Tokenizer::TOKEN_IDIV) ||
        check(Tokenizer::TOKEN_MOD) ||
        check(Tokenizer::TOKEN_AND)) {
        node.addChild(terminalNode(advance()));
    } else {
        syntaxError("multiplicative operator");
    }

    return node;
}
