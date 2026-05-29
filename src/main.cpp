#include "arion/ASTBuilder.hpp"
#include "arion/DecoratedAST.hpp"
#include "arion/IntermediateCodeGenerator.hpp"
#include "arion/Interpreter.hpp"
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

    std::filesystem::path outputDir = "test/milestone-4";
    std::filesystem::create_directories(outputDir);
    std::filesystem::path dastOutputPath = outputDir / ("dast-" + inputPath.filename().string());
    std::filesystem::path codeOutputPath = outputDir / ("ic-" + inputPath.filename().string());
    std::filesystem::path runtimeOutputPath = outputDir / ("output-" + inputPath.filename().string());

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
        arion::DecoratedAST decoratedAstTree(astTree);

        std::ofstream dastOut(dastOutputPath);
        decoratedAstTree.printTable(dastOut);
        decoratedAstTree.printTree(dastOut);

        arion::IntermediateCodeGenerator generator(decoratedAstTree.getSymbolTable());
        arion::IntermediateCode intermediateCode = generator.generate(decoratedAstTree.getASTTree());

        std::ofstream codeOut(codeOutputPath);
        intermediateCode.printCode(codeOut);

        arion::Interpreter interpreter;
        std::ostringstream runtimeOutput;
        interpreter.execute(intermediateCode, runtimeOutput);

        std::ofstream runtimeOut(runtimeOutputPath);
        runtimeOut << runtimeOutput.str();

        if (!runtimeOutput.str().empty()) {
            std::cout << "Program output:" << std::endl;
            std::cout << runtimeOutput.str();
        }

    } catch (const arion::ParserError &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    std::cout << "Decorated AST outputted to " << dastOutputPath << std::endl;
    std::cout << "Intermediate code outputted to " << codeOutputPath << std::endl;
    std::cout << "Interpreter outputted to " << runtimeOutputPath << std::endl;
}
