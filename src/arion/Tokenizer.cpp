#include "Tokenizer.hpp"
#include <fstream>
#include <iostream>
using namespace arion;

Tokenizer::Tokenizer()
{
    dfa_.addState(START, "start");
    dfa_.setStartState(START);

    // ident
    std::string letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string digits = "0123456789";
    dfa_.addState(TOKEN_IDENT, "ident");
    dfa_.setFinalState(TOKEN_IDENT, true);
    dfa_.addTransition(START, letters, TOKEN_IDENT);
    dfa_.addTransition(TOKEN_IDENT, letters + digits, TOKEN_IDENT);

    // intcon
    dfa_.addState(TOKEN_INT, "intcon");
    dfa_.addTransition(START, digits, TOKEN_INT);
    dfa_.addTransition(TOKEN_INT, digits, TOKEN_INT);
    dfa_.setFinalState(TOKEN_INT, true);

    // real
    dfa_.addState(TOKEN_REAL, "realcon");
    dfa_.addState(TOKEN_REAL_PERIOD, "real_period");
    dfa_.addTransition(TOKEN_INT, '.', TOKEN_REAL_PERIOD);
    dfa_.addTransition(TOKEN_REAL_PERIOD, digits, TOKEN_REAL);
    dfa_.addTransition(TOKEN_REAL, digits, TOKEN_REAL);
    dfa_.setFinalState(TOKEN_REAL, true);

    // char & string
    std::string allChars = "";
    for (int i = 1; i < 128; i++) {
        allChars += (char)i;
    }

    dfa_.addState(TOKEN_CHAR_AND_STRING_START, "char_and_string_start");

    // char states
    dfa_.addState(TOKEN_CHAR_CONTENT, "char_content");
    dfa_.addState(TOKEN_CHAR_ESCAPE_OR_END, "char_escape_or_end");
    dfa_.addState(TOKEN_CHAR_END, "char_end");
    dfa_.setFinalState(TOKEN_CHAR_ESCAPE_OR_END, true);
    dfa_.setFinalState(TOKEN_CHAR_END, true);

    // string states
    dfa_.addState(TOKEN_STRING_CONTENT, "string_content");
    dfa_.addState(TOKEN_STRING_ESCAPE_OR_END, "string_escape_or_end");
    dfa_.setFinalState(TOKEN_STRING_ESCAPE_OR_END, true);

    // char and string transition
    dfa_.addTransition(START, '\'', TOKEN_CHAR_AND_STRING_START);

    // char transition
    dfa_.addTransition(TOKEN_CHAR_AND_STRING_START, allChars, TOKEN_CHAR_CONTENT);
    dfa_.removeTransition(TOKEN_CHAR_AND_STRING_START, '\''); // ' untuk escape char
    dfa_.addTransition(TOKEN_CHAR_AND_STRING_START, '\'', TOKEN_CHAR_ESCAPE_OR_END);
    dfa_.addTransition(TOKEN_CHAR_ESCAPE_OR_END, '\'', TOKEN_CHAR_CONTENT);

    // char to string transition
    dfa_.addTransition(TOKEN_CHAR_CONTENT, allChars, TOKEN_STRING_CONTENT);
    dfa_.removeTransition(TOKEN_CHAR_CONTENT, '\'');
    dfa_.addTransition(TOKEN_CHAR_CONTENT, '\'', TOKEN_CHAR_END);

    // string transition
    dfa_.addTransition(TOKEN_STRING_CONTENT, allChars, TOKEN_STRING_CONTENT);
    dfa_.removeTransition(TOKEN_STRING_CONTENT, '\'');
    dfa_.addTransition(TOKEN_STRING_CONTENT, '\'', TOKEN_STRING_ESCAPE_OR_END);
    dfa_.addTransition(TOKEN_STRING_ESCAPE_OR_END, '\'', TOKEN_STRING_CONTENT);

    // operator
    dfa_.addState(TOKEN_PLUS, "plus");
    dfa_.addTransition(START, '+', TOKEN_PLUS);
    dfa_.setFinalState(TOKEN_PLUS, true);

    dfa_.addState(TOKEN_MINUS, "minus");
    dfa_.addTransition(START, '-', TOKEN_MINUS);
    dfa_.setFinalState(TOKEN_MINUS, true);

    dfa_.addState(TOKEN_TIMES, "times");
    dfa_.addTransition(START, '*', TOKEN_TIMES);
    dfa_.setFinalState(TOKEN_TIMES, true);

    dfa_.addState(TOKEN_RDIV, "rdiv");
    dfa_.addTransition(START, '/', TOKEN_RDIV);
    dfa_.setFinalState(TOKEN_RDIV, true);

    // comparator
    dfa_.addState(TOKEN_EQUAL_START, "equal_start");
    dfa_.addState(TOKEN_EQUAL_END, "equal_end");
    dfa_.addTransition(START, '=', TOKEN_EQUAL_START);
    dfa_.addTransition(TOKEN_EQUAL_START, '=', TOKEN_EQUAL_END);
    dfa_.setFinalState(TOKEN_EQUAL_END, true);

    dfa_.addState(TOKEN_LESS_THAN, "less_than");
    dfa_.addState(TOKEN_LESS_THAN_OR_EQUAL, "less_equal");
    dfa_.addState(TOKEN_NOT_EQUAL, "not_equal");

    dfa_.setFinalState(TOKEN_LESS_THAN, true);
    dfa_.addTransition(START, '<', TOKEN_LESS_THAN);

    dfa_.addTransition(TOKEN_LESS_THAN, '=', TOKEN_LESS_THAN_OR_EQUAL);
    dfa_.setFinalState(TOKEN_LESS_THAN_OR_EQUAL, true);

    dfa_.addTransition(TOKEN_LESS_THAN, '>', TOKEN_NOT_EQUAL);
    dfa_.setFinalState(TOKEN_NOT_EQUAL, true);

    dfa_.addState(TOKEN_GREATER_THAN, "greater_than");
    dfa_.addState(TOKEN_GREATER_THAN_OR_EQUAL, "greater_than_or_equal");
    dfa_.setFinalState(TOKEN_GREATER_THAN, true);
    dfa_.setFinalState(TOKEN_GREATER_THAN_OR_EQUAL, true);
    dfa_.addTransition(START, '>', TOKEN_GREATER_THAN);
    dfa_.addTransition(TOKEN_GREATER_THAN, '=', TOKEN_GREATER_THAN_OR_EQUAL);

    // assignment
    dfa_.addState(TOKEN_COLON, "colon_token");
    dfa_.addState(TOKEN_BECOMES, "becomes");

    dfa_.addTransition(START, ':', TOKEN_COLON);
    dfa_.setFinalState(TOKEN_COLON, true);

    dfa_.addTransition(TOKEN_COLON, '=', TOKEN_BECOMES);
    dfa_.setFinalState(TOKEN_BECOMES, true);

    // delimiter & separator
    dfa_.addState(TOKEN_LEFT_PARENTHESES, "lparent");
    dfa_.addTransition(START, '(', TOKEN_LEFT_PARENTHESES);
    dfa_.setFinalState(TOKEN_LEFT_PARENTHESES, true);

    dfa_.addState(TOKEN_RIGHT_PARENTHESES, "rparent");
    dfa_.addTransition(START, ')', TOKEN_RIGHT_PARENTHESES);
    dfa_.setFinalState(TOKEN_RIGHT_PARENTHESES, true);

    dfa_.addState(TOKEN_LEFT_BRACKET, "lbracket");
    dfa_.addTransition(START, '[', TOKEN_LEFT_BRACKET);
    dfa_.setFinalState(TOKEN_LEFT_BRACKET, true);

    dfa_.addState(TOKEN_RIGHT_BRACKET, "rbracket");
    dfa_.addTransition(START, ']', TOKEN_RIGHT_BRACKET);
    dfa_.setFinalState(TOKEN_RIGHT_BRACKET, true);

    dfa_.addState(TOKEN_COMMA, "comma");
    dfa_.addTransition(START, ',', TOKEN_COMMA);
    dfa_.setFinalState(TOKEN_COMMA, true);

    dfa_.addState(TOKEN_SEMICOLON, "semicolon");
    dfa_.addTransition(START, ';', TOKEN_SEMICOLON);
    dfa_.setFinalState(TOKEN_SEMICOLON, true);

    dfa_.addState(TOKEN_PERIOD, "period");
    dfa_.addTransition(START, '.', TOKEN_PERIOD);
    dfa_.setFinalState(TOKEN_PERIOD, true);

    // Comment
    dfa_.addState(TOKEN_COMMENT_CURLY_START, "comment_curly_start");
    dfa_.addState(TOKEN_COMMENT_STAR_START, "comment_star_start");
    dfa_.addTransition(START, '{', TOKEN_COMMENT_CURLY_START);
    dfa_.addTransition(TOKEN_LEFT_PARENTHESES, '*', TOKEN_COMMENT_STAR_START);
    dfa_.addState(TOKEN_COMMENT_CURLY_BODY, "comment_curly_body");
    dfa_.addState(TOKEN_COMMENT_STAR_BODY, "comment_star_body");
    dfa_.addTransition(TOKEN_COMMENT_CURLY_START, allChars, TOKEN_COMMENT_CURLY_BODY);
    dfa_.addTransition(TOKEN_COMMENT_STAR_START, allChars, TOKEN_COMMENT_STAR_BODY);
    dfa_.addTransition(TOKEN_COMMENT_CURLY_BODY, allChars, TOKEN_COMMENT_CURLY_BODY);
    dfa_.addTransition(TOKEN_COMMENT_STAR_BODY, allChars, TOKEN_COMMENT_STAR_BODY);
    dfa_.removeTransition(TOKEN_COMMENT_CURLY_BODY, '}');
    dfa_.removeTransition(TOKEN_COMMENT_STAR_BODY, '*');
    dfa_.addState(TOKEN_COMMENT_CURLY_END, "comment_curly_end");
    dfa_.addState(TOKEN_COMMENT_STAR_END, "comment_star_end");
    dfa_.addState(TOKEN_COMMENT_PARENTHESES_END, "comment_parentheses_end");
    dfa_.addTransition(TOKEN_COMMENT_CURLY_BODY, '}', TOKEN_COMMENT_CURLY_END);
    dfa_.addTransition(TOKEN_COMMENT_STAR_BODY, '*', TOKEN_COMMENT_STAR_END);
    dfa_.addTransition(TOKEN_COMMENT_STAR_END, allChars, TOKEN_COMMENT_STAR_BODY);
    dfa_.removeTransition(TOKEN_COMMENT_STAR_END, ')');
    dfa_.addTransition(TOKEN_COMMENT_STAR_END, ')', TOKEN_COMMENT_PARENTHESES_END);
    dfa_.setFinalState(TOKEN_COMMENT_CURLY_END, true);
    dfa_.setFinalState(TOKEN_COMMENT_PARENTHESES_END, true);
}

void Tokenizer::setStream(const std::string &inputPath)
{
    input_ = std::ifstream(inputPath);
}
void Tokenizer::setStream(std::ifstream &inputStream)
{
    input_ = std::move(inputStream);
}
bool Tokenizer::isStreamOpen()
{
    return input_.is_open();
}
void Tokenizer::closeStream()
{
    input_.close();
}

void Tokenizer::setDebug(bool debug)
{
    debug_ = debug;
}

char Tokenizer::peekChar()
{
    return input_.peek();
}

char Tokenizer::getChar()
{
    return input_.get();
}

std::string Tokenizer::toLower(const std::string &s)
{
    std::string result = s;
    for (char &c : result)
        c = std::tolower(static_cast<unsigned char>(c));
    return result;
}

void Tokenizer::skipWhitespace()
{
    while (isspace(peekChar())) {
        if (debug_) {
            std::cout << peekChar() << " -> [skipped whitespace]" << std::endl;
        }
        getChar();
    }
}

Token Tokenizer::getNextToken()
{
    skipWhitespace();

    dfa_.resetToStartState();
    lexeme_.clear();

    if (peekChar() == EOF) {
        return {TOKEN_EOF, ""};
    }

    int lastFinalState = TOKEN_UNKNOWN;

    char c;
    while (true) {
        c = peekChar();
        if (c == EOF) {
            if (dfa_.isFinalState(dfa_.getCurrentState())) {
                lastFinalState = dfa_.getCurrentState();
            }
            else {
                lastFinalState = TOKEN_EOF;
            }
            break;
        };

        if (!dfa_.canTransition(c)) break;

        if (dfa_.getCurrentState() != TOKEN_CHAR_ESCAPE_OR_END && dfa_.getCurrentState() != TOKEN_STRING_ESCAPE_OR_END) {
            lexeme_ += getChar();
        }
        else {
            getChar();
        }
        dfa_.transition(c);

        if (debug_) {
            std::string lower = toLower(lexeme_);
            auto it = wordTokens_.find(lower);
            if (it != wordTokens_.end()) {
                std::cout << c << " -> "
                          << "State : " << tokenToString({it->second, lexeme_}) << std::endl;
            }
            else {
                std::cout << c << " -> "
                          << "State : " << dfa_.getStateName(dfa_.getCurrentState()) << std::endl;
            }
        }

        if (dfa_.isFinalState(dfa_.getCurrentState())) {
            lastFinalState = dfa_.getCurrentState();
        }
        else {
            lastFinalState = TOKEN_UNKNOWN;
        }
    }

    if (lastFinalState == TOKEN_EOF) {
        if (debug_) {
            std::cout << c << " -> "
                      << "Got Token : " << tokenToString({TOKEN_UNKNOWN, lexeme_}) << std::endl;
        }
        return {TOKEN_EOF, ""};
    }

    if (lastFinalState == TOKEN_UNKNOWN) {
        if (!isspace(c)) {
            lexeme_ += getChar();
        }
        return {TOKEN_UNKNOWN, lexeme_};
    }

    if (lastFinalState == TOKEN_COMMENT_CURLY_END ||
        lastFinalState == TOKEN_COMMENT_PARENTHESES_END) {
        if (debug_) {
            std::cout << c << " -> "
                      << "Comment: " << getLexeme() << std::endl;
        }
        return getNextToken();
    }

    if (lastFinalState == TOKEN_IDENT) {
        std::string lower = toLower(lexeme_);
        auto it = wordTokens_.find(lower);
        if (it != wordTokens_.end()) {
            if (debug_) {
                std::cout << c << " -> "
                          << "Got Token : " << tokenToString({it->second, getLexeme()}) << std::endl;
            }
            return {it->second, lexeme_};
        }
    }

    if (debug_) {
        std::cout << c << " -> "
                  << "Got Token : " << tokenToString({lastFinalState, getLexeme()}) << std::endl;
    }

    return {lastFinalState, lexeme_};
}

std::string Tokenizer::tokenToString(Token type)
{
    switch (type.type) {
        case TOKEN_INT:
            return "intcon (" + type.value + ")";
        case TOKEN_REAL:
            return "realcon (" + type.value + ")";
        case TOKEN_CHAR_END:
            return "charcon (" + type.value + ")";
        case TOKEN_STRING_ESCAPE_OR_END:
            return "string (" + type.value + ")";
        case TOKEN_NOT:
            return "notsy";
        case TOKEN_PLUS:
            return "plus";
        case TOKEN_MINUS:
            return "minus";
        case TOKEN_TIMES:
            return "times";
        case TOKEN_IDIV:
            return "idiv";
        case TOKEN_RDIV:
            return "rdiv";
        case TOKEN_MOD:
            return "imod";
        case TOKEN_AND:
            return "andsy";
        case TOKEN_OR:
            return "orsy";
        case TOKEN_EQUAL_END:
            return "eql";
        case TOKEN_NOT_EQUAL:
            return "neq";
        case TOKEN_GREATER_THAN:
            return "gtr";
        case TOKEN_GREATER_THAN_OR_EQUAL:
            return "geq";
        case TOKEN_LESS_THAN:
            return "lss";
        case TOKEN_LESS_THAN_OR_EQUAL:
            return "leq";
        case TOKEN_LEFT_PARENTHESES:
            return "lparent";
        case TOKEN_RIGHT_PARENTHESES:
            return "rparent";
        case TOKEN_LEFT_BRACKET:
            return "lbrack";
        case TOKEN_RIGHT_BRACKET:
            return "rbrack";
        case TOKEN_COMMA:
            return "comma";
        case TOKEN_SEMICOLON:
            return "semicolon";
        case TOKEN_PERIOD:
            return "period";
        case TOKEN_COLON:
            return "colon";
        case TOKEN_BECOMES:
            return "becomes";
        case TOKEN_CONST:
            return "constsy";
        case TOKEN_TYPE:
            return "typesy";
        case TOKEN_VAR:
            return "varsy";
        case TOKEN_FUNCTION:
            return "functionsy";
        case TOKEN_PROCEDURE:
            return "proceduresy";
        case TOKEN_ARRAY:
            return "arraysy";
        case TOKEN_RECORD:
            return "recordsy";
        case TOKEN_PROGRAM:
            return "programsy";
        case TOKEN_IDENT:
            return "ident (" + type.value + ")";
        case TOKEN_BEGIN:
            return "beginsy";
        case TOKEN_IF:
            return "ifsy";
        case TOKEN_CASE:
            return "casesy";
        case TOKEN_REPEAT:
            return "repeatsy";
        case TOKEN_WHILE:
            return "whilesy";
        case TOKEN_FOR:
            return "forsy";
        case TOKEN_END:
            return "endsy";
        case TOKEN_ELSE:
            return "elsesy";
        case TOKEN_UNTIL:
            return "untilsy";
        case TOKEN_OF:
            return "ofsy";
        case TOKEN_DO:
            return "dosy";
        case TOKEN_TO:
            return "tosy";
        case TOKEN_DOWNTO:
            return "downtosy";
        case TOKEN_THEN:
            return "thensy";
        case TOKEN_UNKNOWN:
            return "unknown (" + type.value + ")";
        default:
            return "unknown (" + type.value + ")";
    };
}