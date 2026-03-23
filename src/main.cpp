#include "arion/Tokenizer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

int main() {
    std::ifstream in("input.txt");
    std::ofstream out("output.txt");

    std::stringstream buffer;
    buffer << in.rdbuf();

    arion::Tokenizer tokenizer(buffer.str());

    while (true) {
        arion::Token t = tokenizer.getNextToken();

        if (t.type == tokenizer.TOKEN_EOF) break;

        std::string result = tokenizer.tokenToString(t, tokenizer.getLexeme());

        if (!result.empty()) {
            out << result << "\n";
        }
    }
}