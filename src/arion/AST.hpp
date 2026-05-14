#ifndef ARION_AST_H
#define ARION_AST_H

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace arion {
    struct ASTChild;

    enum class ASTNodeKind {
        Program,
        Declarations,
        ConstDeclarations,
        ConstDeclaration,
        TypeDeclarations,
        TypeDeclaration,
        VarDeclarations,
        VarDeclaration,
        FieldDeclaration,
        ProcedureDeclaration,
        FunctionDeclaration,
        Parameters,
        ParameterGroup,
        Parameter,
        Block,
        CompoundStatement,
        StatementList,
        EmptyStatement,
        Assignment,
        IfStatement,
        CaseStatement,
        CaseBranch,
        WhileStatement,
        RepeatStatement,
        ForStatement,
        ProcedureCall,
        FunctionCall,
        Arguments,
        BinaryOperation,
        UnaryOperation,
        Variable,
        ArrayAccess,
        FieldAccess,
        IntegerLiteral,
        RealLiteral,
        CharLiteral,
        StringLiteral,
        BooleanLiteral,
        NamedType,
        ReturnType,
        ArrayType,
        RecordType,
        RangeType,
        EnumeratedType,
        Identifier,
        Unknown
    };

    enum class ASTChildRole {
        None,
        Declaration,
        Const,
        Type,
        Variable,
        Field,
        Parameter,
        Group,
        Parameters,
        ReturnType,
        Block,
        Body,
        Statement,
        Target,
        Value,
        Condition,
        Then,
        Else,
        Expression,
        Branch,
        Label,
        Left,
        Right,
        Base,
        Index,
        Element,
        Low,
        High,
        Arg,
        Start,
        End
    };

    struct ASTAnnotation {
        std::string typeName;
        int tabIndex = -1;
        int blockIndex = -1;
        int arrayIndex = -1;
        int lexicalLevel = -1;

    };

    class ASTNode {
    public:
        explicit ASTNode(ASTNodeKind kind = ASTNodeKind::Unknown);

        ASTNode &addChild(ASTNode child);
        ASTNode &addChild(ASTChildRole role, ASTNode child);
        ASTNode &setAttribute(std::string key, std::string value);

        ASTNodeKind getKind() const;
        std::string getAttribute(const std::string &key) const;
        const std::vector<ASTChild> &getChildren() const;
        const std::vector<std::pair<std::string, std::string>> &getAttributes() const;
        const ASTNode &childAt(std::size_t index) const;
        ASTNode &childAt(std::size_t index);
        const ASTNode *childWithRole(ASTChildRole role) const;

        void setAnnotation(ASTAnnotation annotation);
        ASTAnnotation &annotation();
        const ASTAnnotation &annotation() const;

        static std::string kindToString(ASTNodeKind kind);
        static std::string roleToString(ASTChildRole role);

    private:
        ASTNodeKind kind_;
        ASTAnnotation annotation_;
        std::vector<std::pair<std::string, std::string>> attributes_;
        std::vector<ASTChild> children_;

    };

    struct ASTChild {
        ASTChildRole role;
        ASTNode node;
    };
}

#endif
