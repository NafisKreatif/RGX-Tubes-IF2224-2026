#include "DecoratedAST.hpp"
#include "AST.hpp"

#include <iomanip>

using namespace arion;

DecoratedAST::DecoratedAST(const ASTNode &astTree) : astTree_{std::move(astTree)} {
    buildTableAndDecorateTree();
}

void DecoratedAST::buildTableAndDecorateTree() {
    dfs(astTree_);
}

void DecoratedAST::dfs(ASTNode &astNode) {
    decorateNode(astNode);

    for (size_t i = 0; i < astNode.getChildren().size(); i++) {
        dfs(astNode.childAt(i));
    }
}

void DecoratedAST::decorateNode(ASTNode &astNode) {
    switch (astNode.getKind()) {
        case ASTNodeKind::Program:
            decorateProgram(astNode);
            break;

        case ASTNodeKind::Declarations:
            // decorateDeclarations(astNode);
            break;

        case ASTNodeKind::ConstDeclarations:
            // decorateConstDeclarations(astNode);
            break;

        case ASTNodeKind::ConstDeclaration:
            decorateConstDeclaration(astNode);
            break;

        case ASTNodeKind::TypeDeclarations:
            // decorateTypeDeclarations(astNode);
            break;

        case ASTNodeKind::TypeDeclaration:
            decorateTypeDeclaration(astNode);
            break;

        case ASTNodeKind::VarDeclarations:
            // decorateVarDeclarations(astNode);
            break;

        case ASTNodeKind::VarDeclaration:
            decorateVarDeclaration(astNode);
            break;

        case ASTNodeKind::FieldDeclaration:
            // decorateFieldDeclaration(astNode);
            break;

        case ASTNodeKind::ProcedureDeclaration:
            decorateProcedureDeclaration(astNode);
            break;

        case ASTNodeKind::FunctionDeclaration:
            decorateFunctionDeclaration(astNode);
            break;

        case ASTNodeKind::Parameters:
            // decorateParameters(astNode);
            break;

        case ASTNodeKind::ParameterGroup:
            decorateParameterGroup(astNode);
            break;

        case ASTNodeKind::Parameter:
            // decorateParameter(astNode);
            break;

        case ASTNodeKind::Block:
            decorateBlock(astNode);
            break;

        case ASTNodeKind::CompoundStatement:
            decorateCompoundStatement(astNode);
            break;

        case ASTNodeKind::StatementList:
            decorateStatementList(astNode);
            break;

        case ASTNodeKind::EmptyStatement:
            // decorateEmptyStatement(astNode);
            break;

        case ASTNodeKind::Assignment:
            // decorateAssignment(astNode);
            break;

        case ASTNodeKind::IfStatement:
            decorateIfStatement(astNode);
            break;

        case ASTNodeKind::CaseStatement:
            decorateCaseStatement(astNode);
            break;

        case ASTNodeKind::CaseBranch:
            // decorateCaseBranch(astNode);
            break;

        case ASTNodeKind::WhileStatement:
            decorateWhileStatement(astNode);
            break;

        case ASTNodeKind::RepeatStatement:
            decorateRepeatStatement(astNode);
            break;

        case ASTNodeKind::ForStatement:
            decorateForStatement(astNode);
            break;

        case ASTNodeKind::ProcedureCall:
            // decorateProcedureCall(astNode);
            break;

        case ASTNodeKind::FunctionCall:
            // decorateFunctionCall(astNode);
            break;

        case ASTNodeKind::Arguments:
            // decorateArguments(astNode);
            break;

        case ASTNodeKind::BinaryOperation:
            // decorateBinaryOperation(astNode);
            break;

        case ASTNodeKind::UnaryOperation:
            // decorateUnaryOperation(astNode);
            break;

        case ASTNodeKind::Variable:
            decorateVariable(astNode);
            break;

        case ASTNodeKind::ArrayAccess:
            // decorateArrayAccess(astNode);
            break;

        case ASTNodeKind::FieldAccess:
            // decorateFieldAccess(astNode);
            break;

        case ASTNodeKind::IntegerLiteral:
            // decorateIntegerLiteral(astNode);
            break;

        case ASTNodeKind::RealLiteral:
            // decorateRealLiteral(astNode);
            break;

        case ASTNodeKind::CharLiteral:
            // decorateCharLiteral(astNode);
            break;

        case ASTNodeKind::StringLiteral:
            // decorateStringLiteral(astNode);
            break;

        case ASTNodeKind::BooleanLiteral:
            // decorateBooleanLiteral(astNode);
            break;

        case ASTNodeKind::NamedType:
            // decorateNamedType(astNode);
            break;

        case ASTNodeKind::ReturnType:
            // decorateReturnType(astNode);
            break;

        case ASTNodeKind::ArrayType:
            decorateArrayType(astNode);
            break;

        case ASTNodeKind::RecordType:
            decorateRecordType(astNode);
            break;

        case ASTNodeKind::RangeType:
            // decorateRangeType(astNode);
            break;

        case ASTNodeKind::EnumeratedType:
            // decorateEnumeratedType(astNode);
            break;

        case ASTNodeKind::Identifier:
            // decorateIdentifier(astNode);
            break;

        case ASTNodeKind::Unknown:
            // decorateUnknown(astNode);
            break;
    }
}

void DecoratedAST::decorateProgram(ASTNode &node) {
    int index = symbolTable_.declareProgram(node.getAttribute("name"));
    ASTAnnotation annotation;
    annotation.tabIndex = index;
    annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
    node.setAnnotation(annotation);
}

static TypeKind nodeKindLiteralToTypeKind(ASTNodeKind kind) {
    switch (kind) {
        case ASTNodeKind::IntegerLiteral:
            return TypeKind::Integer;
        case ASTNodeKind::RealLiteral:
            return TypeKind::Real;
        case ASTNodeKind::CharLiteral:
            return TypeKind::Char;
        case ASTNodeKind::StringLiteral:
            return TypeKind::String;
        case ASTNodeKind::BooleanLiteral:
            return TypeKind::Boolean;
    }
    return TypeKind::Unknown;
}

void DecoratedAST::decorateConstDeclaration(ASTNode &node) {
    const ASTNode *valueNode = node.childWithRole(ASTChildRole::Value);

    std::string name = node.getAttribute("name");
    TypeKind typeKind;
    std::string typeName;
    std::string value;

    if (valueNode->getKind() != ASTNodeKind::UnaryOperation) {
        typeKind = nodeKindLiteralToTypeKind(valueNode->getKind());
        typeName = ASTNode::kindToString(valueNode->getKind());
        value = valueNode->getAttribute("value");
    }
    else {
        const ASTNode *unaryValueNode = valueNode->childWithRole(ASTChildRole::Expression);
        typeKind = nodeKindLiteralToTypeKind(unaryValueNode->getKind());
        typeName = ASTNode::kindToString(valueNode->getKind());
        value = valueNode->getAttribute("name") + unaryValueNode->getAttribute("value");
    }

    int index = symbolTable_.declareConstant(name, typeKind, value);
    ASTAnnotation annotation;
    annotation.typeName = typeName;
    annotation.tabIndex = index;
    annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
    node.setAnnotation(annotation);
    // std::cout << "Declared constant: " << name << ' ' << typeName << ' ' << value << std::endl;
}

void DecoratedAST::decorateTypeDeclaration(ASTNode &node) {
    const ASTNode *typeNode = node.childWithRole(ASTChildRole::Type);

    std::string name = node.getAttribute("name");

    if (typeNode->getKind() == ASTNodeKind::NamedType) {
        std::string typeName = typeNode->getAttribute("name");

        auto tabEntry = symbolTable_.requireType(typeName);
        int ref = symbolTable_.requireTypeIndex(typeName);
        symbolTable_.declareType(name, tabEntry.type, ref);
    }
    else if (typeNode->getKind() == ASTNodeKind::ArrayType) {
        std::cout << "Start decorating array type: " << name << '\n';
        const ASTNode *indexNode = typeNode->childWithRole(ASTChildRole::Index);
        const ASTNode *elementNode = typeNode->childWithRole(ASTChildRole::Element);

        if (indexNode == nullptr || elementNode == nullptr) {
            throw std::runtime_error("Can't get index or element of array: " + name);
        }

        std::string indexName = indexNode->getAttribute("name");
        TypeKind indexType;
        int indexRef = 0;
        std::string low = "";
        std::string high = "";

        if (indexNode->getKind() == ASTNodeKind::Identifier) {
            auto indexTabEntry = symbolTable_.requireType(indexName);
            indexRef = symbolTable_.requireTypeIndex(indexName);

            if (indexTabEntry.type == TypeKind::Subrange) {
                auto typeDescriptor = symbolTable_.requireTypeDescriptor(indexTabEntry.ref);
                low = typeDescriptor.low;
                high = typeDescriptor.high;
                indexType = typeDescriptor.baseType;
            }
            else {
                throw std::runtime_error("Identifier is not an array range: " + name);
            }
        }
        else if (indexNode->getKind() == ASTNodeKind::RangeType) {
            const ASTNode *lowNode = indexNode->childWithRole(ASTChildRole::Low);
            const ASTNode *highNode = indexNode->childWithRole(ASTChildRole::High);
            low = lowNode->getAttribute("value");
            high = highNode->getAttribute("value");
            TypeKind lowType = nodeKindLiteralToTypeKind(lowNode->getKind());
            TypeKind highType = nodeKindLiteralToTypeKind(highNode->getKind());
            if (lowType != highType) {
                throw std::runtime_error("Range low and high type is not the same: " + name);
            }
            indexType = lowType;
        }

        if (indexType == TypeKind::Real) {
            throw std::runtime_error("Range cannot be a real number: " + name);
        }

        std::string elementName = elementNode->getAttribute("name");
        auto elementTabEntry = symbolTable_.requireType(elementName);
        int elementRef = symbolTable_.requireTypeIndex(elementName);

        // std::cout << "Declaring array type: " << name << ' ' << SymbolTable::typeKindToString(indexType)
        //           << ' ' << indexRef << ' ' << low << ' ' << high << ' '
        //           << SymbolTable::typeKindToString(elementTabEntry.type) << ' ' << elementRef << '\n';

        symbolTable_.declareArrayType(name, indexType, indexRef, low, high, elementTabEntry.type, elementRef);
    }
    else if (typeNode->getKind() == ASTNodeKind::RangeType) {
        const ASTNode *lowNode = typeNode->childWithRole(ASTChildRole::Low);
        const ASTNode *highNode = typeNode->childWithRole(ASTChildRole::High);
        std::string low = lowNode->getAttribute("value");
        std::string high = highNode->getAttribute("value");

        TypeKind lowType = nodeKindLiteralToTypeKind(lowNode->getKind());
        TypeKind highType = nodeKindLiteralToTypeKind(highNode->getKind());
        if (lowType != highType) {
            throw std::runtime_error("Range low and high type is not the same: " + name);
        }

        TypeKind baseKind = lowType;
        int baseRef = symbolTable_.requireTypeIndex(SymbolTable::typeKindToString(lowType));

        symbolTable_.declareSubrangeType(name, baseKind, low, high, baseRef);
    }
    else if (typeNode->getKind() == ASTNodeKind::EnumeratedType) {
        std::vector<std::string> values;
        for (auto &&child : typeNode->getChildren()) {
            const ASTNode &enumNode = child.node;
            values.push_back(enumNode.getAttribute("name"));
        }
        symbolTable_.declareEnumeratedType(name, values);
    }
    else if (typeNode->getKind() == ASTNodeKind::RecordType) {
    }
}

void DecoratedAST::decorateVarDeclaration(ASTNode &node) {}
void DecoratedAST::decorateType(ASTNode &node) {}
void DecoratedAST::decorateArrayType(ASTNode &node) {}
void DecoratedAST::decorateRange(ASTNode &node) {}
void DecoratedAST::decorateEnumerated(ASTNode &node) {}
void DecoratedAST::decorateRecordType(ASTNode &node) {}
void DecoratedAST::decorateFieldList(ASTNode &node) {}
void DecoratedAST::decorateFieldPart(ASTNode &node) {}
void DecoratedAST::decorateSubprogramDeclaration(ASTNode &node) {}
void DecoratedAST::decorateProcedureDeclaration(ASTNode &node) {}
void DecoratedAST::decorateFunctionDeclaration(ASTNode &node) {}
void DecoratedAST::decorateBlock(ASTNode &node) {}
void DecoratedAST::decorateFormalParameterList(ASTNode &node) {}
void DecoratedAST::decorateParameterGroup(ASTNode &node) {}
void DecoratedAST::decorateCompoundStatement(ASTNode &node) {}
void DecoratedAST::decorateStatementList(ASTNode &node) {}
void DecoratedAST::decorateStatement(ASTNode &node) {}
void DecoratedAST::decorateAssignmentStatement(ASTNode &node) {}
void DecoratedAST::decorateIfStatement(ASTNode &node) {}
void DecoratedAST::decorateCaseStatement(ASTNode &node) {}
void DecoratedAST::decorateCaseBranches(ASTNode &node) {}
void DecoratedAST::decorateWhileStatement(ASTNode &node) {}
void DecoratedAST::decorateRepeatStatement(ASTNode &node) {}
void DecoratedAST::decorateForStatement(ASTNode &node) {}
void DecoratedAST::decorateProcedureOrFunctionCall(ASTNode &node) {}
void DecoratedAST::decorateParameterList(ASTNode &node) {}
void DecoratedAST::decorateExpression(ASTNode &node) {}
void DecoratedAST::decorateSimpleExpression(ASTNode &node) {}
void DecoratedAST::decorateTerm(ASTNode &node) {}
void DecoratedAST::decorateFactor(ASTNode &node) {}
void DecoratedAST::decorateVariable(ASTNode &node) {}
void DecoratedAST::decorateIndexList(ASTNode &node) {}
void DecoratedAST::decorateConstant(ASTNode &node) {}

void DecoratedAST::printTable(std::ostream &out) const {
    out << "tab:\n";
    out << symbolTable_.dumpTab() << "\n";
    out << "atab:\n";
    out << symbolTable_.dumpATab() << "\n";
    out << "btab:\n";
    out << symbolTable_.dumpBTab() << "\n";
    out << "type:\n";
    out << symbolTable_.dumpTypeDescriptors() << "\n";
}

void DecoratedAST::printTree(std::ostream &out) const {
    std::vector<bool> isLast = {};
    printTreeHelper(astTree_, 0, isLast, out);
}

void DecoratedAST::printTreeHelper(const ASTNode &node, int depth, std::vector<bool> &isLast, std::ostream &out) const {
    for (int i = 0; i < depth; i++) {
        out << ((i == depth - 1)
                    ? (isLast[i] ? "└── " : "├── ")
                    : (isLast[i] ? "    " : "│   "));
    }
    out << ASTNode::kindToString(node.getKind());
    auto attributes = node.getAttributes();
    if (attributes.size() > 0) {
        out << "(";
        for (int i = 0; i < (int)attributes.size(); i++) {
            out << attributes[i].first << ": " << attributes[i].second;
            if (i == (int)attributes.size() - 1) {
                out << ")";
            }
            else {
                out << ", ";
            }
        }
    }
    out << "\n";

    auto children = node.getChildren();
    for (int i = 0; i < (int)children.size(); i++) {
        isLast.push_back(i == ((int)children.size() - 1));
        printTreeHelper(children[i].node, depth + 1, isLast, out);
        isLast.pop_back();
    }
}