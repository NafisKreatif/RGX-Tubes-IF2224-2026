#ifndef ARION_TOKENIZER_H
#define ARION_TOKENIZER_H
#include "DFA.hpp"
#include <fstream>
#include <vector>

namespace arion {
    struct Token {
        int type;
        std::string value;
    };
    class Tokenizer {

    public:
        enum State {
            START = 1000,
            TOKEN_INT,
            TOKEN_REAL_PERIOD,
            TOKEN_REAL,
            TOKEN_CHAR_AND_STRING_START,
            TOKEN_CHAR_CONTENT,
            TOKEN_CHAR_END,
            TOKEN_CHAR_ESCAPE_OR_END,
            TOKEN_STRING_CONTENT,
            TOKEN_STRING_ESCAPE_CHAR,
            TOKEN_STRING_ESCAPE_OR_END,
            TOKEN_PLUS,
            TOKEN_MINUS,
            TOKEN_TIMES,
            TOKEN_RDIV,
            TOKEN_EQUAL_START,
            TOKEN_EQUAL_END,
            TOKEN_NOT_EQUAL,
            TOKEN_GREATER_THAN,
            TOKEN_GREATER_THAN_OR_EQUAL,
            TOKEN_LESS_THAN,
            TOKEN_LESS_THAN_OR_EQUAL,
            TOKEN_LEFT_PARENTHESES,
            TOKEN_RIGHT_PARENTHESES,
            TOKEN_LEFT_BRACKET,
            TOKEN_RIGHT_BRACKET,
            TOKEN_COMMA,
            TOKEN_SEMICOLON,
            TOKEN_PERIOD,
            TOKEN_COLON,
            TOKEN_BECOMES,
            TOKEN_IDENT,
            TOKEN_NOT,
            TOKEN_IDIV,
            TOKEN_MOD,
            TOKEN_AND,
            TOKEN_OR,
            TOKEN_CONST,
            TOKEN_TYPE,
            TOKEN_VAR,
            TOKEN_FUNCTION,
            TOKEN_PROCEDURE,
            TOKEN_ARRAY,
            TOKEN_RECORD,
            TOKEN_PROGRAM,
            TOKEN_BEGIN,
            TOKEN_IF,
            TOKEN_CASE,
            TOKEN_REPEAT,
            TOKEN_WHILE,
            TOKEN_FOR,
            TOKEN_END,
            TOKEN_ELSE,
            TOKEN_UNTIL,
            TOKEN_OF,
            TOKEN_DO,
            TOKEN_TO,
            TOKEN_DOWNTO,
            TOKEN_THEN,
            TOKEN_COMMENT_CURLY_START,
            TOKEN_COMMENT_PARENTHESES_START,
            TOKEN_COMMENT_STAR_START,
            TOKEN_COMMENT_CURLY_BODY,
            TOKEN_COMMENT_STAR_BODY,
            TOKEN_COMMENT_CURLY_END,
            TOKEN_COMMENT_STAR_END,
            TOKEN_COMMENT_PARENTHESES_END,
            TOKEN_EOF,
            TOKEN_UNKNOWN
        };
        Tokenizer();

        void setStream(const std::string &);
        void setStream(std::ifstream &);
        bool isStreamOpen();
        void closeStream();

        void setDebug(bool);
        Token getNextToken();
        std::vector<Token> tokenizeAll();
        std::string getLexeme() { return lexeme_; };
        
        static std::string tokenToString(Token type);

    private:
        char peekChar();
        char getChar();
        void skipWhitespace();
        std::string toLower(const std::string &);

        bool debug_ = false;
        DFA dfa_;
        std::ifstream input_;
        std::string lexeme_;
        std::unordered_map<std::string, int> wordTokens_ = {
            {"not", TOKEN_NOT},
            {"div", TOKEN_IDIV},
            {"mod", TOKEN_MOD},
            {"and", TOKEN_AND},
            {"or", TOKEN_OR},
            {"const", TOKEN_CONST},
            {"type", TOKEN_TYPE},
            {"var", TOKEN_VAR},
            {"function", TOKEN_FUNCTION},
            {"procedure", TOKEN_PROCEDURE},
            {"array", TOKEN_ARRAY},
            {"record", TOKEN_RECORD},
            {"program", TOKEN_PROGRAM},
            {"begin", TOKEN_BEGIN},
            {"if", TOKEN_IF},
            {"case", TOKEN_CASE},
            {"repeat", TOKEN_REPEAT},
            {"while", TOKEN_WHILE},
            {"for", TOKEN_FOR},
            {"end", TOKEN_END},
            {"else", TOKEN_ELSE},
            {"until", TOKEN_UNTIL},
            {"of", TOKEN_OF},
            {"do", TOKEN_DO},
            {"to", TOKEN_TO},
            {"downto", TOKEN_DOWNTO},
            {"then", TOKEN_THEN}};
    };
}

#endif
