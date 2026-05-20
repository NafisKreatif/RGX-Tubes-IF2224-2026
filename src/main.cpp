#include "arion/ASTBuilder.hpp"
#include "arion/DecoratedAST.hpp"
#include "arion/Parser.hpp"
#include "arion/Tokenizer.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

int main(int argc, char **argv) {
    std::filesystem::path inputPath = "test/input.txt";
    std::cout << "Input file path: ";
    std::cin >> inputPath;
    std::ifstream in(inputPath);
    if (!in.is_open()) {
        std::cout << "File not found" << std::endl;
        return 1;
    }

    std::filesystem::path outputPath = "test/milestone-3/decorated-ast-" + inputPath.filename().string();
    std::ofstream out(outputPath);

    arion::Tokenizer tokenizer;
    tokenizer.setStream(in);
    if (argc > 1 && std::string(argv[1]) == "debug") {
        tokenizer.setDebug(true);
    }

    try {
        std::vector<arion::Token> tokens = tokenizer.tokenizeAll();
        arion::Parser parser(tokens);
        arion::ParseNode parseResult = parser.parse();
        arion::ASTBuilder builder;

        arion::ASTNode astTree = builder.build(parseResult);
        astTree.printTree(out);

        arion::DecoratedAST decoratedAstTree(astTree);

        decoratedAstTree.printTable(out);
        decoratedAstTree.printTree(out);

    } catch (const arion::ParserError &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    std::cout << "Outputted to " << outputPath << std::endl;
}
