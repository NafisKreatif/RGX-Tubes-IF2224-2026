#include "arion/Tokenizer.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

int main(int argc, char **argv)
{
    std::filesystem::path inputPath = "test/input.txt";
    std::cout << "Input file path: ";
    std::cin >> inputPath;
    std::ifstream in(inputPath);
    if (!in.is_open()) {
        std::cout << "File not found" << std::endl;
        return 1;
    }

    std::filesystem::path outputPath = "test/milestone-1/tokenized-" + inputPath.filename().string();
    std::ofstream out(outputPath);

    arion::Tokenizer tokenizer;
    tokenizer.setStream(in);
    if (argc > 1 && std::string(argv[1]) == "debug") {
        tokenizer.setDebug(true);
    }
    while (true) {
        arion::Token t = tokenizer.getNextToken();

        if (t.type == tokenizer.TOKEN_EOF) break;

        std::string result = tokenizer.tokenToString(t);

        if (!result.empty()) {
            out << result << "\n";
        }
    }

    std::cout << "Outputted to " << outputPath << std::endl;
}