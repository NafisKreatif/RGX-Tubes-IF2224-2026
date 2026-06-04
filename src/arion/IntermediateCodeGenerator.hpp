#ifndef ARION_INTERMEDIATE_CODE_GENERATOR_H
#define ARION_INTERMEDIATE_CODE_GENERATOR_H

#include "AST.hpp"
#include "IntermediateCode.hpp"
#include "SymbolTable.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace arion {
    class IntermediateCodeGenerator {
    public:
        explicit IntermediateCodeGenerator(const SymbolTable &symbolTable);

        IntermediateCode generate(const ASTNode &decoratedAst);

    private:
        const SymbolTable &symbolTable_;
        IntermediateCode code_;
        int currentBlockIndex_ = 0;
        std::unordered_map<int, int> tabOwnerBlock_;
        std::unordered_map<int, int> tabRuntimeAddress_;
        std::unordered_map<int, int> frameSizes_;
        std::unordered_map<int, int> functionReturnAddress_;
        std::unordered_map<int, int> subprogramEntryLines_;
        std::vector<std::pair<std::size_t, int>> pendingCallPatches_;

        void generateProgram(const ASTNode &node);
        void generateSubprogramDeclarations(const ASTNode &node);
        void generateSubprogram(const ASTNode &node);
        void generateNode(const ASTNode &node);
        void generateStatement(const ASTNode &node);
        void generateAssignment(const ASTNode &node);
        void generateIfStatement(const ASTNode &node);
        void generateCaseStatement(const ASTNode &node);
        void generateWhileStatement(const ASTNode &node);
        void generateRepeatStatement(const ASTNode &node);
        void generateForStatement(const ASTNode &node);
        void generateProcedureCall(const ASTNode &node);
        void generateFunctionCall(const ASTNode &node);
        void generateExpression(const ASTNode &node);
        void generateVariableLoad(const ASTNode &node);
        void generateVariableStore(const ASTNode &node);
        void generateVariableAddress(const ASTNode &node);

        void prepareRuntimeLayout();
        void mapBlockStorage(int blockIndex, int &nextAddress);
        void patchPendingCalls();
        std::vector<int> tabEntriesForBlock(int blockIndex) const;
        std::vector<const ASTNode *> callArguments(const ASTNode &node) const;
        int frameSize(const ASTNode &programNode) const;
        int frameSizeForBlock(int blockIndex) const;
        int nodeTabIndex(const ASTNode &node) const;
        int nodeAddress(const ASTNode &node) const;
        std::pair<int, int> variableAccessAddress(const ASTNode &node) const;
        bool fixedVariableAccessAddress(const ASTNode &node, int &level, int &address) const;
        std::pair<TypeKind, int> nodeValueType(const ASTNode &node) const;
        int fieldTabIndex(const ASTNode &node) const;
        bool staticIndexOrdinal(const ASTNode &node, int &ordinal) const;
        bool literalOrdinal(TypeKind type, const std::string &value, int &ordinal) const;
        int tabAddress(int tabIndex) const;
        int tabBlockIndex(int tabIndex) const;
        int activationBlockIndex(int blockIndex) const;
        int lexicalLevelDiffToBlock(int targetActivationBlockIndex) const;
        int lexicalLevelDiffForTab(int tabIndex) const;
        int callableLevelDiff(int calleeBlockIndex) const;
        int lookupVisibleTabIndex(const std::string &name) const;
        int emitCall(const TabEntry &entry, const std::string &name, std::size_t argumentCount);
        std::string literalValue(const ASTNode &node) const;
        OperationCode binaryOperation(const ASTNode &node) const;
        std::string operatorText(const ASTNode &node) const;
        std::string normalizeIdentifier(const std::string &identifier) const;
        bool sameIdentifier(const std::string &left, const std::string &right) const;
        bool isWriteProcedure(const ASTNode &node) const;
        bool isWritelnProcedure(const ASTNode &node) const;
        bool isActivationBlock(BlockKind kind) const;
        bool isFunctionReturnTarget(int tabIndex) const;
        const ASTNode *requiredChild(const ASTNode &node, ASTChildRole role) const;
    };

    class IntermediateCodeGeneratorError : public std::runtime_error {
    public:
        explicit IntermediateCodeGeneratorError(const std::string &message);
    };
}

#endif
