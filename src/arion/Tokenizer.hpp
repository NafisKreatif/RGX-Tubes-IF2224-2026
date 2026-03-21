#ifndef ARION_TOKENIZER_H
#define ARION_TOKENIZER_H
#include "DFA.hpp"

namespace arion {
    class Tokenizer {

    public:
        enum State {
            START,
            TOKEN_STRING_START,
            TOKEN_STRING_CHAR,
            TOKEN_STRING_ESCAPE_CHAR,
            TOKEN_STRING_END,
            // ... dsb.
        };
        Tokenizer();

    private:
        DFA dfa_;
    };
}

#endif
