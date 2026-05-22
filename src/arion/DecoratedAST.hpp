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

        size_t anonymousTypeCount_ = 0;

        void dfs(ASTNode &astNode);
        void buildTableAndDecorateTree();

        bool decorateNode(ASTNode &node); // Return need to recursively decorate or no
        void decorateProgram(ASTNode &node);
        void decorateConstDeclaration(ASTNode &node);
        void decorateTypeDeclaration(ASTNode &node);
        void decorateFieldDeclaration(ASTNode &node, int recordBlockRef);
        std::pair<int, TypeKind> decorateAnonymousType(ASTNode &node);
        void decorateVarDeclaration(ASTNode &node);
        void decorateProcedureDeclaration(ASTNode &node);
        void decorateFunctionDeclaration(ASTNode &node);
        void decorateBlock(ASTNode &node, std::string name = "block");
        void decorateParameter(ASTNode &node);
        void decorateAssignmentStatement(ASTNode &node);
        void decorateIfStatement(ASTNode &node);
        void decorateCaseStatement(ASTNode &node);
        void decorateCaseBranches(ASTNode &node);
        void decorateWhileStatement(ASTNode &node);
        void decorateRepeatStatement(ASTNode &node);
        void decorateForStatement(ASTNode &node);
        void decorateProcedureCall(ASTNode &node);
        std::pair<int, TypeKind> decorateFunctionCall(ASTNode &node);
        std::pair<int, TypeKind> decorateBinaryOperator(ASTNode &node);
        std::pair<int, TypeKind> decorateUnaryOperator(ASTNode &node);
        std::pair<int, TypeKind> decorateVariable(ASTNode &node);
        std::pair<int, TypeKind> decorateArrayAccess(ASTNode &node);
        std::pair<int, TypeKind> decorateFieldAccess(ASTNode &node);
        std::pair<int, TypeKind> decorateExpression(ASTNode &node);

        bool isAssignmentCompatible(int typeRef1, TypeKind type1, int typeRef2, TypeKind type2);
        bool isTypeCompatible(int typeRef1, TypeKind type1, int typeRef2, TypeKind type2);
        bool isOrderedType(TypeKind type);
        bool isNumberType(TypeKind type);
        bool isStringType(TypeKind type);
        bool isBooleanType(TypeKind type);
        bool isBuiltinWriteLikeProcedure(const TabEntry &entry);
        bool isPrintableType(TypeKind type);

        void printTreeHelper(const ASTNode &node, int depth, std::vector<bool> &isLast, std::ostream &out = std::cout, ASTChildRole role = ASTChildRole::None) const;

    public:
        DecoratedAST(const ASTNode &astTree);

        const ASTNode &getASTTree() const;
        const SymbolTable &getSymbolTable() const;

        void printTable(std::ostream &out = std::cout) const;
        void printTree(std::ostream &out = std::cout) const;
    };
}

#endif
