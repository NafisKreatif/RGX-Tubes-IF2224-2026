#ifndef ARION_AST_BUILDER_H
#define ARION_AST_BUILDER_H

#include "AST.hpp"
#include "Parser.hpp"
#include <stdexcept>
#include <string>
#include <vector>

namespace arion {
    class ASTBuilder {
    public:
        ASTNode build(const ParseNode &parseTree) const;

    private:
        ASTNode buildNode(const ParseNode &node) const;
        ASTNode buildProgram(const ParseNode &node) const;
        ASTNode buildDeclarationPart(const ParseNode &node) const;
        ASTNode buildConstDeclaration(const ParseNode &node) const;
        ASTNode buildTypeDeclaration(const ParseNode &node) const;
        ASTNode buildVarDeclaration(const ParseNode &node) const;
        ASTNode buildType(const ParseNode &node) const;
        ASTNode buildArrayType(const ParseNode &node) const;
        ASTNode buildRange(const ParseNode &node) const;
        ASTNode buildEnumerated(const ParseNode &node) const;
        ASTNode buildRecordType(const ParseNode &node) const;
        ASTNode buildFieldList(const ParseNode &node) const;
        ASTNode buildFieldPart(const ParseNode &node) const;
        ASTNode buildSubprogramDeclaration(const ParseNode &node) const;
        ASTNode buildProcedureDeclaration(const ParseNode &node) const;
        ASTNode buildFunctionDeclaration(const ParseNode &node) const;
        ASTNode buildBlock(const ParseNode &node) const;
        ASTNode buildFormalParameterList(const ParseNode &node) const;
        ASTNode buildParameterGroup(const ParseNode &node) const;
        ASTNode buildCompoundStatement(const ParseNode &node) const;
        ASTNode buildStatementList(const ParseNode &node) const;
        ASTNode buildStatement(const ParseNode &node) const;
        ASTNode buildAssignmentStatement(const ParseNode &node) const;
        ASTNode buildIfStatement(const ParseNode &node) const;
        ASTNode buildCaseStatement(const ParseNode &node) const;
        std::vector<ASTNode> buildCaseBranches(const ParseNode &node) const;
        ASTNode buildWhileStatement(const ParseNode &node) const;
        ASTNode buildRepeatStatement(const ParseNode &node) const;
        ASTNode buildForStatement(const ParseNode &node) const;
        ASTNode buildProcedureOrFunctionCall(const ParseNode &node, bool asFunction) const;
        ASTNode buildParameterList(const ParseNode &node) const;
        ASTNode buildExpression(const ParseNode &node) const;
        ASTNode buildSimpleExpression(const ParseNode &node) const;
        ASTNode buildTerm(const ParseNode &node) const;
        ASTNode buildFactor(const ParseNode &node) const;
        ASTNode buildVariable(const ParseNode &node) const;
        std::vector<ASTNode> buildIndexList(const ParseNode &node) const;
        ASTNode buildConstant(const ParseNode &node) const;

        std::vector<std::string> collectIdentifierList(const ParseNode &node) const;
        const ParseNode *firstChildWithLabel(const ParseNode &node, const std::string &label) const;
        std::string terminalValue(const ParseNode &node) const;
        std::string operatorText(const ParseNode &node) const;
        ASTNode terminalAsLiteralOrIdentifier(const ParseNode &node) const;
        ASTNode makeIdentifier(const std::string &name) const;
        bool hasLabel(const ParseNode &node, const std::string &label) const;
        bool isTerminal(const ParseNode &node, int tokenType) const;
    };

    class ASTBuilderError : public std::runtime_error {
    public:
        explicit ASTBuilderError(const std::string &message);
    };
}

#endif