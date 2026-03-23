#include "Tokenizer.hpp"
using namespace arion;

// TODO: initialize all the state and transition to the dfa_
Tokenizer::Tokenizer(std::string in) : input(in)
{
    dfa_.addState(START, "start");
    dfa_.setStartState(START);

    //ident 
    std::string letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string digits  = "0123456789";
    dfa_.addState(TOKEN_IDENT, "identifier");
    dfa_.setFinalState(TOKEN_IDENT, true);
    dfa_.addTransition(START, letters, TOKEN_IDENT);
    dfa_.addTransition(TOKEN_IDENT, letters + digits, TOKEN_IDENT);

    //intcon
    dfa_.addState(TOKEN_INT, "intcon");
    dfa_.addTransition(START,digits,TOKEN_INT);
    dfa_.addTransition(TOKEN_INT, digits, TOKEN_INT);
    dfa_.setFinalState(TOKEN_INT, true);

    //real
    dfa_.addState(TOKEN_REAL, "realcon");
    dfa_.addState(TOKEN_REAL_PERIOD, "real_period");
    dfa_.addTransition(TOKEN_INT, '.', TOKEN_REAL_PERIOD);
    dfa_.addTransition(TOKEN_REAL_PERIOD, digits, TOKEN_REAL);
    dfa_.setFinalState(TOKEN_REAL, true);
    


    //char & string
    std::string allChars = "";
    for (int i = 1; i < 128; i++) {
        allChars += (char)i;
    }
    
    dfa_.addState(TOKEN_CHAR_AND_STRING_START, "char_and_string_start");
    
    //char states
    dfa_.addState(TOKEN_CHAR_CONTENT, "char_content");
    dfa_.addState(TOKEN_ESCAPE_CHAR, "escape_char");
    dfa_.addState(TOKEN_CHAR_END,"char_end");
    dfa_.setFinalState(TOKEN_CHAR_END, true);

    //string states
    dfa_.addState(TOKEN_STRING_CONTENT, "string_content");
    dfa_.addState(TOKEN_STRING_ESCAPE_CHAR, "string_escape_char");
    dfa_.addState(TOKEN_STRING_END, "string_end");
    dfa_.setFinalState(TOKEN_STRING_END, true);

    //char and string transition
    dfa_.addTransition(START, '\'', TOKEN_CHAR_AND_STRING_START);
    //char transition
    dfa_.addTransition(TOKEN_CHAR_AND_STRING_START, allChars, TOKEN_CHAR_CONTENT);
    dfa_.removeTransition(TOKEN_CHAR_AND_STRING_START, "\'\\"); // Selain ' dan \ boleh dimasukin string
    dfa_.addTransition(TOKEN_CHAR_AND_STRING_START, '\\', TOKEN_ESCAPE_CHAR);
    dfa_.addTransition(TOKEN_ESCAPE_CHAR, allChars, TOKEN_CHAR_CONTENT);
    
    //string transition
    dfa_.addTransition(TOKEN_CHAR_CONTENT, allChars, TOKEN_STRING_CONTENT);
    dfa_.removeTransition(TOKEN_CHAR_CONTENT, '\'');
    dfa_.addTransition(TOKEN_CHAR_CONTENT, '\'', TOKEN_CHAR_END);
    dfa_.addTransition(TOKEN_STRING_CONTENT, allChars, TOKEN_STRING_CONTENT);
    dfa_.removeTransition(TOKEN_STRING_CONTENT, '\'');
    dfa_.removeTransition(TOKEN_STRING_CONTENT, '\\');
    dfa_.addTransition(TOKEN_STRING_CONTENT, '\\', TOKEN_STRING_ESCAPE_CHAR);
    dfa_.addTransition(TOKEN_STRING_ESCAPE_CHAR, allChars, TOKEN_STRING_CONTENT);
    dfa_.addTransition(TOKEN_STRING_CONTENT, '\'', TOKEN_STRING_END);
    

    //operator
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

    //comparator
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

    //assignment
    dfa_.addState(TOKEN_COLON, "colon_token");
    dfa_.addState(TOKEN_BECOMES, "becomes");

    dfa_.addTransition(START, ':', TOKEN_COLON);
    dfa_.setFinalState(TOKEN_COLON, true);

    dfa_.addTransition(TOKEN_COLON, '=', TOKEN_BECOMES);
    dfa_.setFinalState(TOKEN_BECOMES, true);

    //delimiter & separator
    dfa_.addState(TOKEN_LEFT_PARENTHESES, "lparen");
    dfa_.addTransition(START, '(', TOKEN_LEFT_PARENTHESES);
    dfa_.setFinalState(TOKEN_LEFT_PARENTHESES, true);

    dfa_.addState(TOKEN_RIGHT_PARENTHESES, "rparen");
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

        //Comment
    dfa_.addState(TOKEN_COMMENT_CURLY_START, "comment_curly_start");
    dfa_.addState(TOKEN_COMMENT_STAR_START, "comment_star_start");
    dfa_.addTransition(START, '{', TOKEN_COMMENT_CURLY_START);
    dfa_.addTransition(TOKEN_LEFT_PARENTHESES, '*', TOKEN_COMMENT_STAR_START);
    dfa_.addState(TOKEN_COMMENT_CURLY_BODY, "comment_curly_body");
    dfa_.addState(TOKEN_COMMENT_STAR_BODY, "comment_star_body");
    dfa_.addTransition(TOKEN_COMMENT_CURLY_START, allChars, TOKEN_COMMENT_CURLY_BODY);
    dfa_.addTransition(TOKEN_COMMENT_STAR_START, allChars, TOKEN_COMMENT_STAR_BODY);
    dfa_.addTransition(TOKEN_COMMENT_CURLY_BODY, allChars, TOKEN_COMMENT_CURLY_BODY);
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
char Tokenizer::peekChar() {
    if (pos >= (int)input.size()) return '\0';
    return input[pos];
}

char Tokenizer::getChar() {
    if (pos >=(int)input.size()) return '\0';
    return input[pos++];
}

void Tokenizer::retract() {
    if (pos > 0) pos--;
}
std::string Tokenizer::toLower(const std::string& s) {
    std::string result = s;
    for (char &c : result)
        c = std::tolower(static_cast<unsigned char>(c));
    return result;
}
void Tokenizer::skipWhitespace() {
    while (isspace(peekChar())) getChar();
}

Token Tokenizer::getNextToken() {
    skipWhitespace();

    dfa_.resetToStartState();
    lexeme.clear();

    if (peekChar() == '\0') {
        return {TOKEN_EOF, ""};
    }

    int startPos = pos;
    int lastFinalState = -1;
    int lastFinalPos = pos;

    while (true) {
        char c = peekChar();
        if (c == '\0') break;

        if (!dfa_.canTransition(c)) break;

        dfa_.transition(c);
        getChar();

        if (dfa_.isFinalState(dfa_.getCurrentState())) {
            lastFinalState = dfa_.getCurrentState();
            lastFinalPos = pos;
        }
    }

    if (lastFinalState == -1) {
        if (pos >= (int)input.size()) {
            return {TOKEN_EOF, ""};
        }
        char unknown = getChar(); // consume!
        return {TOKEN_UNKNOWN, std::string(1, unknown)};
    }

    pos = lastFinalPos;
    lexeme = input.substr(startPos, lastFinalPos - startPos);

    if (lastFinalState == TOKEN_COMMENT_CURLY_END ||
        lastFinalState == TOKEN_COMMENT_PARENTHESES_END) {
        return getNextToken();
    }

    if (lastFinalState == TOKEN_IDENT) {
        std::string lower = toLower(lexeme);
        auto it = wordTokens.find(lower);
        if (it != wordTokens.end()) {
            return {it->second, lexeme};
        }
    }

    return {lastFinalState, lexeme};
}

std::string Tokenizer::tokenToString(Token type, const std::string& lexeme) {
    switch (type.type) {
        case TOKEN_INT: return "intcon (" + lexeme + ")";
        case TOKEN_REAL: return "realcon (" + lexeme + ")";
        case TOKEN_CHAR_END : return "charcon (" + lexeme + ")";
        case TOKEN_STRING_END: return "string (" + lexeme + ")";
        case TOKEN_NOT: return "notsy";
        case TOKEN_PLUS: return "plus";
        case TOKEN_MINUS: return "minus";
        case TOKEN_TIMES: return "times";
        case TOKEN_IDIV: return "idiv";
        case TOKEN_RDIV: return "rdiv";
        case TOKEN_MOD: return "imod";
        case TOKEN_AND: return "andsy";
        case TOKEN_OR: return "orsy";
        case TOKEN_EQUAL_END: return "eql";
        case TOKEN_NOT_EQUAL: return "neq";
        case TOKEN_GREATER_THAN: return "gtr";
        case TOKEN_GREATER_THAN_OR_EQUAL: return "geq";
        case TOKEN_LESS_THAN: return "lss";
        case TOKEN_LESS_THAN_OR_EQUAL: return "leq";
        case TOKEN_LEFT_PARENTHESES: return "lparent";
        case TOKEN_RIGHT_PARENTHESES: return "rparent";
        case TOKEN_LEFT_BRACKET: return "lbrack";
        case TOKEN_RIGHT_BRACKET: return "rbrack";
        case TOKEN_COMMA: return "comma";
        case TOKEN_SEMICOLON: return "semicolon";
        case TOKEN_PERIOD: return "period";
        case TOKEN_COLON: return "colon";
        case TOKEN_BECOMES: return "becomes";
        case TOKEN_CONST: return "constsy";
        case TOKEN_TYPE: return "typesy";
        case TOKEN_VAR: return "varsy";
        case TOKEN_FUNCTION: return "functionsy";
        case TOKEN_PROCEDURE: return "proceduresy";
        case TOKEN_ARRAY: return "arraysy";
        case TOKEN_RECORD: return "recordsy";
        case TOKEN_PROGRAM: return "programsy";
        case TOKEN_IDENT: return "ident (" + lexeme + ")";
        case TOKEN_BEGIN: return "beginsy";
        case TOKEN_IF: return "ifsy";
        case TOKEN_CASE: return "casesy";
        case TOKEN_REPEAT: return "repeatsy";
        case TOKEN_WHILE: return "whilesy";
        case TOKEN_FOR: return "forsy";
        case TOKEN_END: return "endsy";
        case TOKEN_ELSE: return "elsesy";
        case TOKEN_UNTIL: return "untilsy";
        case TOKEN_OF: return "ofsy";
        case TOKEN_DO: return "dosy";
        case TOKEN_TO: return "tosy";
        case TOKEN_DOWNTO: return "downtosy";
        case TOKEN_THEN: return "thensy";
        case TOKEN_UNKNOWN: return "unknown (" + type.value + ")";
        default: return "unknown (" + lexeme + ")";
    };
}