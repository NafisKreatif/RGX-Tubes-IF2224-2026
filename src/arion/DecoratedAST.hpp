#ifndef ARION_DECORATED_AST_H
#define ARION_DECORATED_AST_H

#include "AST.hpp"
#include "SymbolTable.hpp"
#include <iostream>

namespace arion
{
    class DecoratedAST {
    private:
        ASTNode astTree_;
        SymbolTable symbolTable_;

        size_t array_parameter_count = 0;
         
        void dfs(ASTNode &astNode);
        void buildTableAndDecorateTree();

        bool decorateNode(ASTNode &node); // Return need to recursively decorate or no

        void decorateProgram(ASTNode &node);
        void decorateConstDeclaration(ASTNode &node);
        void decorateTypeDeclaration(ASTNode &node);
        void decorateVarDeclaration(ASTNode &node);
        void decorateProcedureDeclaration(ASTNode &node);
        void decorateFunctionDeclaration(ASTNode &node);
        void decorateBlock(ASTNode &node);
        void decorateFormalParameterList(ASTNode &node);
        void decorateParameter(ASTNode &node);
        void decorateCompoundStatement(ASTNode &node);
        void decorateStatementList(ASTNode &node);
        void decorateStatement(ASTNode &node);
        void decorateAssignmentStatement(ASTNode &node);
        void decorateIfStatement(ASTNode &node);
        void decorateCaseStatement(ASTNode &node);
        void decorateCaseBranches(ASTNode &node);
        void decorateWhileStatement(ASTNode &node);
        void decorateRepeatStatement(ASTNode &node);
        void decorateForStatement(ASTNode &node);
        void decorateProcedureOrFunctionCall(ASTNode &node);
        void decorateParameterList(ASTNode &node);
        void decorateExpression(ASTNode &node);
        void decorateSimpleExpression(ASTNode &node);
        void decorateTerm(ASTNode &node);
        void decorateFactor(ASTNode &node);
        void decorateVariable(ASTNode &node);
        void decorateIndexList(ASTNode &node);
        void decorateConstant(ASTNode &node);

        void printTreeHelper(const ASTNode& node, int depth, std::vector<bool> &isLast, std::ostream& out = std::cout) const;

    public:
        DecoratedAST(const ASTNode &astTree);

        const ASTNode &getASTTree() const;
        const SymbolTable &getSymbolTable() const;

        void printTable(std::ostream& out = std::cout) const;
        void printTree(std::ostream& out = std::cout) const;
    };
}

#endif
