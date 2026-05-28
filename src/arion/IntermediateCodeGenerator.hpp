#ifndef ARION_INTERMEDIATE_CODE_GENERATOR_H
#define ARION_INTERMEDIATE_CODE_GENERATOR_H

#include "AST.hpp"
#include "IntermediateCode.hpp"
#include "SymbolTable.hpp"

#include <stdexcept>
#include <string>

namespace arion {
    class IntermediateCodeGenerator {
    public:
        explicit IntermediateCodeGenerator(const SymbolTable &symbolTable);

        IntermediateCode generate(const ASTNode &decoratedAst);

    private:
        const SymbolTable &symbolTable_;
        IntermediateCode code_;

        void generateProgram(const ASTNode &node);
        void generateNode(const ASTNode &node);
        void generateStatement(const ASTNode &node);
        void generateAssignment(const ASTNode &node);
        void generateIfStatement(const ASTNode &node);
        void generateCaseStatement(const ASTNode &node);
        void generateWhileStatement(const ASTNode &node);
        void generateRepeatStatement(const ASTNode &node);
        void generateForStatement(const ASTNode &node);
        void generateProcedureCall(const ASTNode &node);
        void generateExpression(const ASTNode &node);
        void generateVariableLoad(const ASTNode &node);
        void generateVariableStore(const ASTNode &node);

        int frameSize(const ASTNode &programNode) const;
        int nodeTabIndex(const ASTNode &node) const;
        int nodeAddress(const ASTNode &node) const;
        int tabAddress(int tabIndex) const;
        std::string literalValue(const ASTNode &node) const;
        OperationCode binaryOperation(const ASTNode &node) const;
        std::string operatorText(const ASTNode &node) const;
        bool isWriteProcedure(const ASTNode &node) const;
        bool isWritelnProcedure(const ASTNode &node) const;
        const ASTNode *requiredChild(const ASTNode &node, ASTChildRole role) const;
    };

    class IntermediateCodeGeneratorError : public std::runtime_error {
    public:
        explicit IntermediateCodeGeneratorError(const std::string &message);
    };
}

#endif
