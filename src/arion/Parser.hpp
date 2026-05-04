#ifndef ARION_PARSER_H
#define ARION_PARSER_H

#include "Tokenizer.hpp"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace arion {
    class ParseNode {
    public:
        ParseNode() = default;
        explicit ParseNode(std::string label);

        ParseNode &addChild(ParseNode child);
        const std::string &getLabel() const;
        const std::vector<ParseNode> &getChildren() const;
        std::string toString(int depth = 0) const;

    private:
        std::string label_;
        std::vector<ParseNode> children_;
    };

    class Parser {
    public:
        Parser() = default;
        explicit Parser(std::vector<Token> tokens);

        void setTokens(std::vector<Token> tokens);
        ParseNode parse();

    private:
        enum NonTerminal {
            PROGRAM,
            PROGRAM_HEADER,
            DECLARATION_PART,
            CONST_DECLARATION,
            CONSTANT,
            TYPE_DECLARATION,
            VAR_DECLARATION,
            IDENTIFIER_LIST,
            TYPE,
            ARRAY_TYPE,
            RANGE,
            ENUMERATED,
            RECORD_TYPE,
            FIELD_LIST,
            FIELD_PART,
            SUBPROGRAM_DECLARATION,
            PROCEDURE_DECLARATION,
            FUNCTION_DECLARATION,
            BLOCK,
            FORMAL_PARAMETER_LIST,
            PARAMETER_GROUP,
            COMPOUND_STATEMENT,
            STATEMENT_LIST,
            STATEMENT,
            ASSIGNMENT_STATEMENT,
            IF_STATEMENT,
            CASE_STATEMENT,
            CASE_BLOCK,
            WHILE_STATEMENT,
            REPEAT_STATEMENT,
            FOR_STATEMENT,
            PROCEDURE_FUNCTION_CALL,
            PARAMETER_LIST,
            EXPRESSION,
            SIMPLE_EXPRESSION,
            TERM,
            FACTOR,
            VARIABLE,
            COMPONENT_VARIABLE,
            INDEX_LIST,
            RELATIONAL_OPERATOR,
            ADDITIVE_OPERATOR,
            MULTIPLICATIVE_OPERATOR
        };

        const Token &peek() const;
        const Token &peekNext() const;
        const Token &previous() const;
        bool isAtEnd() const;
        bool check(int tokenType) const;
        bool match(int tokenType);
        Token advance();
        Token expect(int tokenType);

        ParseNode variableNode(NonTerminal variable) const;
        ParseNode terminalNode(const Token &token) const;
        std::string variableName(NonTerminal variable) const;
        std::string terminalName(int tokenType) const;
        std::string tokenName(const Token &token) const;
        void syntaxError(const std::string &expectedName) const;
        void skipIgnoredTokens();

        ParseNode parseProgram();
        ParseNode parseProgramHeader();
        ParseNode parseDeclarationPart();
        ParseNode parseConstDeclaration();
        ParseNode parseConstant();
        ParseNode parseTypeDeclaration();
        ParseNode parseVarDeclaration();
        ParseNode parseIdentifierList();
        ParseNode parseType();
        ParseNode parseArrayType();
        ParseNode parseRange();
        ParseNode parseEnumerated();
        ParseNode parseRecordType();
        ParseNode parseFieldList();
        ParseNode parseFieldPart();
        ParseNode parseSubprogramDeclaration();
        ParseNode parseProcedureDeclaration();
        ParseNode parseFunctionDeclaration();
        ParseNode parseBlock();
        ParseNode parseFormalParameterList();
        ParseNode parseParameterGroup();
        ParseNode parseCompoundStatement();
        ParseNode parseStatementList();
        ParseNode parseStatement();
        ParseNode parseAssignmentStatement();
        ParseNode parseIfStatement();
        ParseNode parseCaseStatement();
        ParseNode parseCaseBlock();
        ParseNode parseWhileStatement();
        ParseNode parseRepeatStatement();
        ParseNode parseForStatement();
        ParseNode parseProcedureOrFunctionCall();
        ParseNode parseParameterList();
        ParseNode parseExpression();
        ParseNode parseSimpleExpression();
        ParseNode parseTerm();
        ParseNode parseFactor();
        ParseNode parseVariable();
        ParseNode parseComponentVariable();
        ParseNode parseIndexList();
        ParseNode parseRelationalOperator();
        ParseNode parseAdditiveOperator();
        ParseNode parseMultiplicativeOperator();

        std::vector<Token> tokens_;
        std::size_t current_ = 0;
        Token eofToken_{Tokenizer::TOKEN_EOF, ""};
    };

    class ParserError : public std::runtime_error {
    public:
        explicit ParserError(const std::string &message);
    };
}

#endif
