#include "Tokenizer.hpp"
using namespace arion;

// TODO: initialize all the state and transition to the dfa_
Tokenizer::Tokenizer()
{
    dfa_.addState(START, "start");
    dfa_.setStartState(START);

    dfa_.addState(TOKEN_STRING_START, "string_start");
    dfa_.addState(TOKEN_STRING_CHAR, "string_char");
    dfa_.addState(TOKEN_STRING_ESCAPE_CHAR, "string_escape_char");
    dfa_.addState(TOKEN_STRING_END, "string_end");
    dfa_.setFinalState(TOKEN_STRING_END, true);

    dfa_.addTransition(START, '\'', TOKEN_STRING_START);
    std::string allChars = "";
    for (int i = 1; i < 128; i++) {
        allChars += (char)i;
    }
    dfa_.addTransition(TOKEN_STRING_START, allChars, TOKEN_STRING_CHAR);
    dfa_.removeTransition(TOKEN_STRING_START, "\'\\"); // Selain ' dan \ boleh dimasukin string
    dfa_.addTransition(TOKEN_STRING_CHAR, '\\', TOKEN_STRING_ESCAPE_CHAR);
    dfa_.addTransition(TOKEN_STRING_ESCAPE_CHAR, allChars, TOKEN_STRING_CHAR);
    dfa_.addTransition(TOKEN_STRING_CHAR, '\'', TOKEN_STRING_END);
}