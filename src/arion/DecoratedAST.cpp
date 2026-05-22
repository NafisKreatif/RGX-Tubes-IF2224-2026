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
    if (decorateNode(astNode)) {
        for (size_t i = 0; i < astNode.getChildren().size(); i++) {
            dfs(astNode.childAt(i));
        }
    }
}

bool DecoratedAST::decorateNode(ASTNode &astNode) {
    switch (astNode.getKind()) {
        case ASTNodeKind::Program:
            decorateProgram(astNode);
            return true;

        case ASTNodeKind::ConstDeclaration:
            decorateConstDeclaration(astNode);
            return false;

        case ASTNodeKind::TypeDeclaration:
            decorateTypeDeclaration(astNode);
            return false;

        case ASTNodeKind::VarDeclaration:
            decorateVarDeclaration(astNode);
            return false;

        case ASTNodeKind::ProcedureDeclaration:
            decorateProcedureDeclaration(astNode);
            return false;

        case ASTNodeKind::FunctionDeclaration:
            decorateFunctionDeclaration(astNode);
            return false;

        case ASTNodeKind::Parameter:
            decorateParameter(astNode);
            return false;

        case ASTNodeKind::Block:
            decorateBlock(astNode);
            return false;

        case ASTNodeKind::EmptyStatement:
            return true;

        case ASTNodeKind::Assignment:
            decorateAssignmentStatement(astNode);
            return true;

        case ASTNodeKind::IfStatement:
            decorateIfStatement(astNode);
            return true;

        case ASTNodeKind::CaseStatement:
            decorateCaseStatement(astNode);
            return true;

        case ASTNodeKind::CaseBranch:
            // decorateCaseBranch(astNode);
            return true;

        case ASTNodeKind::WhileStatement:
            decorateWhileStatement(astNode);
            return true;

        case ASTNodeKind::RepeatStatement:
            decorateRepeatStatement(astNode);
            return true;

        case ASTNodeKind::ForStatement:
            decorateForStatement(astNode);
            return true;

        case ASTNodeKind::ProcedureCall:
            decorateProcedureCall(astNode);
            return false;

        case ASTNodeKind::FunctionCall:
            // decorateFunctionCall(astNode);
            return true;

        case ASTNodeKind::Arguments:
            // decorateArguments(astNode);
            return true;

        case ASTNodeKind::ReturnType:
            // decorateReturnType(astNode);
            return true;

        case ASTNodeKind::Identifier:
            // decorateIdentifier(astNode);
            return true;

        default:
            return true;
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
        default:
            return TypeKind::Unknown;
    }
}

static bool isLiteralKind(ASTNodeKind kind) {
    switch (kind) {
        case ASTNodeKind::IntegerLiteral:
            return true;
        case ASTNodeKind::RealLiteral:
            return true;
        case ASTNodeKind::CharLiteral:
            return true;
        case ASTNodeKind::StringLiteral:
            return true;
        case ASTNodeKind::BooleanLiteral:
            return true;
        default:
            return false;
    }
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
    annotation.typeName = symbolTable_.typeKindToString(typeKind);
    annotation.tabIndex = index;
    annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
    node.setAnnotation(annotation);
}
void DecoratedAST::decorateTypeDeclaration(ASTNode &node) {
    ASTNode *typeNode = node.childWithRole(ASTChildRole::Type);
    std::string name = node.getAttribute("name");

    if (typeNode->getKind() == ASTNodeKind::NamedType) {
        std::string typeName = typeNode->getAttribute("name");

        const TabEntry &tabEntry = symbolTable_.requireType(typeName);
        int tabIndex = symbolTable_.declareType(name, tabEntry.type, tabEntry.ref);
        ASTAnnotation annotation;
        annotation.typeName = typeName;
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
    else if (typeNode->getKind() == ASTNodeKind::ArrayType) {
        const ASTNode *indexNode = typeNode->childWithRole(ASTChildRole::Index);
        ASTNode *elementNode = typeNode->childWithRole(ASTChildRole::Element);

        if (indexNode == nullptr || elementNode == nullptr) {
            throw std::runtime_error("Can't get index or element of array: " + name);
        }

        std::string indexName = indexNode->getAttribute("name");
        TypeKind indexType;
        int indexRef = 0;
        std::string low = "";
        std::string high = "";

        if (indexNode->getKind() == ASTNodeKind::Identifier) {
            const TabEntry &indexTabEntry = symbolTable_.requireType(indexName);
            indexRef = symbolTable_.requireTypeIndex(indexName);

            if (indexTabEntry.type == TypeKind::Subrange) {
                const TypeDescriptor &typeDescriptor = symbolTable_.requireTypeDescriptor(indexTabEntry.ref);
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

        auto [elementRef, elementType] = decorateAnonymousType(*elementNode);
        int tabIndex = symbolTable_.declareArrayType(name, indexType, indexRef, low, high, elementType, elementRef);
        const TabEntry tabEntry = symbolTable_.tab().at(tabIndex);

        ASTAnnotation annotation;
        annotation.typeName = "array";
        annotation.tabIndex = tabIndex;
        annotation.arrayIndex = tabEntry.ref;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
    else if (typeNode->getKind() == ASTNodeKind::RangeType) {
        const ASTNode *lowNode = typeNode->childWithRole(ASTChildRole::Low);
        const ASTNode *highNode = typeNode->childWithRole(ASTChildRole::High);
        std::string low;
        std::string high;
        TypeKind lowType = TypeKind::Unknown;
        TypeKind highType = TypeKind::Unknown;

        if (isLiteralKind(lowNode->getKind())) {
            low = lowNode->getAttribute("value");
            lowType = nodeKindLiteralToTypeKind(lowNode->getKind());
        }
        else if (lowNode->getKind() == ASTNodeKind::Variable) {
            std::string typeName = lowNode->getAttribute("name");
            const TabEntry &tabEntry = symbolTable_.requireLookup(typeName);
            low = tabEntry.value;
            lowType = tabEntry.type;
        }

        if (isLiteralKind(highNode->getKind())) {
            high = highNode->getAttribute("value");
            highType = nodeKindLiteralToTypeKind(highNode->getKind());
        }
        else if (highNode->getKind() == ASTNodeKind::Variable) {
            std::string typeName = highNode->getAttribute("name");
            const TabEntry &tabEntry = symbolTable_.requireLookup(typeName);
            high = tabEntry.value;
            highType = tabEntry.type;
        }

        if (lowType == TypeKind::Unknown || highType == TypeKind::Unknown) {
            throw std::runtime_error("Range low and high type is unknown");
        }
        if (lowType != highType) {
            throw std::runtime_error("Range low and high type is not the same: " + name);
        }

        TypeKind baseKind = lowType;
        int baseRef = symbolTable_.requireTypeIndex(SymbolTable::typeKindToString(lowType));

        int tabIndex = symbolTable_.declareSubrangeType(name, baseKind, low, high, baseRef);
        ASTAnnotation annotation;
        annotation.typeName = "subrange";
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
    else if (typeNode->getKind() == ASTNodeKind::EnumeratedType) {
        std::vector<std::string> values;
        for (auto &&child : typeNode->getChildren()) {
            const ASTNode &enumNode = child.node;
            values.push_back(enumNode.getAttribute("name"));
        }
        int tabIndex = symbolTable_.declareEnumeratedType(name, values);
        ASTAnnotation annotation;
        annotation.typeName = "enumerated";
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
    else if (typeNode->getKind() == ASTNodeKind::RecordType) {
        int recordRef = symbolTable_.beginRecordType(name);
        for (int i = 0; i < (int)typeNode->getChildren().size(); i++) {
            ASTNode &fieldNode = typeNode->childAt(i);
            if (fieldNode.getKind() != ASTNodeKind::FieldDeclaration) continue;
            decorateFieldDeclaration(fieldNode, recordRef);
        }
        symbolTable_.endRecordType();
        int recordTabIndex = symbolTable_.declareRecordType(name, recordRef);

        ASTAnnotation annotation;
        annotation.typeName = "record";
        annotation.tabIndex = recordTabIndex;
        annotation.blockIndex = recordRef;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
}
void DecoratedAST::decorateFieldDeclaration(ASTNode &node, int recordBlockRef) {
    ASTNode *typeNode = node.childWithRole(ASTChildRole::Type);
    std::string name = node.getAttribute("name");

    if (typeNode->getKind() == ASTNodeKind::NamedType) {
        std::string typeName = typeNode->getAttribute("name");

        const TabEntry &tabEntry = symbolTable_.requireType(typeName);
        int tabIndex = symbolTable_.declareField(name, tabEntry.type, recordBlockRef);
        ASTAnnotation annotation;
        annotation.typeName = typeName;
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
    else if (typeNode->getKind() == ASTNodeKind::ArrayType) {
        const ASTNode *indexNode = typeNode->childWithRole(ASTChildRole::Index);
        ASTNode *elementNode = typeNode->childWithRole(ASTChildRole::Element);

        if (indexNode == nullptr || elementNode == nullptr) {
            throw std::runtime_error("Can't get index or element of array: " + name);
        }

        std::string indexName = indexNode->getAttribute("name");
        TypeKind indexType;
        int indexRef = 0;
        std::string low = "";
        std::string high = "";

        if (indexNode->getKind() == ASTNodeKind::Identifier) {
            const TabEntry &indexTabEntry = symbolTable_.requireType(indexName);
            indexRef = symbolTable_.requireTypeIndex(indexName);

            if (indexTabEntry.type == TypeKind::Subrange) {
                const TypeDescriptor &typeDescriptor = symbolTable_.requireTypeDescriptor(indexTabEntry.ref);
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

        auto [elementRef, elementType] = decorateAnonymousType(*elementNode);
        int arrIndex = symbolTable_.addArrayType(indexType, indexRef, low, high, elementType, elementRef);
        int tabIndex = symbolTable_.declareField(name, TypeKind::Array, arrIndex);

        ASTAnnotation annotation;
        annotation.typeName = "array";
        annotation.tabIndex = tabIndex;
        annotation.arrayIndex = arrIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
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
        int subrangeIndex = symbolTable_.declareSubrangeType("_anonymousType" + std::to_string(anonymousTypeCount_++), baseKind, low, high, baseRef);
        int tabIndex = symbolTable_.declareField(name, TypeKind::Subrange, subrangeIndex);

        ASTAnnotation annotation;
        annotation.typeName = "subrange";
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
    else if (typeNode->getKind() == ASTNodeKind::EnumeratedType) {
        std::vector<std::string> values;
        for (auto &&child : typeNode->getChildren()) {
            const ASTNode &enumNode = child.node;
            values.push_back(enumNode.getAttribute("name"));
        }
        int enumIndex = symbolTable_.declareEnumeratedType("_anonymousType" + std::to_string(anonymousTypeCount_++), values);
        int tabIndex = symbolTable_.declareField(name, TypeKind::Enumerated, enumIndex);

        ASTAnnotation annotation;
        annotation.typeName = "enumerated";
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
    else if (typeNode->getKind() == ASTNodeKind::RecordType) {
        int recordRef = symbolTable_.beginRecordType(name);
        for (int i = 0; i < (int)typeNode->getChildren().size(); i++) {
            ASTNode &fieldNode = typeNode->childAt(i);
            if (fieldNode.getKind() != ASTNodeKind::FieldDeclaration) continue;
            decorateFieldDeclaration(fieldNode, recordRef);
        }
        symbolTable_.endRecordType();
        int tabIndex = symbolTable_.declareField(name, TypeKind::Record, recordRef);

        ASTAnnotation annotation;
        annotation.typeName = "record";
        annotation.tabIndex = tabIndex;
        annotation.blockIndex = recordRef;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
}
std::pair<int, TypeKind> DecoratedAST::decorateAnonymousType(ASTNode &node) {
    ASTNode &typeNode = node;

    if (typeNode.getKind() == ASTNodeKind::NamedType) {
        std::string typeName = typeNode.getAttribute("name");

        const TabEntry &tabEntry = symbolTable_.requireType(typeName);
        int ref = symbolTable_.requireTypeIndex(typeName);

        ASTAnnotation annotation;
        annotation.typeName = typeName;
        annotation.tabIndex = ref;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);

        return {ref, tabEntry.type};
    }
    else if (typeNode.getKind() == ASTNodeKind::ArrayType) {
        const ASTNode *indexNode = typeNode.childWithRole(ASTChildRole::Index);
        ASTNode *elementNode = typeNode.childWithRole(ASTChildRole::Element);

        if (indexNode == nullptr || elementNode == nullptr) {
            throw std::runtime_error("Can't get index or element of array");
        }

        std::string indexName = indexNode->getAttribute("name");
        TypeKind indexType;
        int indexRef = 0;
        std::string low = "";
        std::string high = "";

        if (indexNode->getKind() == ASTNodeKind::Identifier) {
            const TabEntry &indexTabEntry = symbolTable_.requireType(indexName);
            indexRef = symbolTable_.requireTypeIndex(indexName);

            if (indexTabEntry.type == TypeKind::Subrange) {
                const TypeDescriptor &typeDescriptor = symbolTable_.requireTypeDescriptor(indexTabEntry.ref);
                low = typeDescriptor.low;
                high = typeDescriptor.high;
                indexType = typeDescriptor.baseType;
            }
            else {
                throw std::runtime_error("Identifier is not an array range");
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
                throw std::runtime_error("Range low and high type is not the same");
            }
            indexType = lowType;
        }

        auto [elementRef, elementType] = decorateAnonymousType(*elementNode);
        int arrRef = symbolTable_.addArrayType(indexType, indexRef, low, high, elementType, elementRef);

        ASTAnnotation annotation;
        annotation.typeName = "array";
        annotation.arrayIndex = arrRef;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);

        return {arrRef, TypeKind::Array};
    }
    else if (typeNode.getKind() == ASTNodeKind::RangeType) {
        const ASTNode *lowNode = typeNode.childWithRole(ASTChildRole::Low);
        const ASTNode *highNode = typeNode.childWithRole(ASTChildRole::High);
        std::string low = lowNode->getAttribute("value");
        std::string high = highNode->getAttribute("value");

        TypeKind lowType = nodeKindLiteralToTypeKind(lowNode->getKind());
        TypeKind highType = nodeKindLiteralToTypeKind(highNode->getKind());
        if (lowType != highType) {
            throw std::runtime_error("Range low and high type is not the same");
        }

        TypeKind baseKind = lowType;
        int baseRef = symbolTable_.requireTypeIndex(SymbolTable::typeKindToString(lowType));

        int tabIndex = symbolTable_.declareSubrangeType("_anonymousType" + std::to_string(anonymousTypeCount_++), baseKind, low, high, baseRef);
        ASTAnnotation annotation;
        annotation.typeName = "subrange";
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);

        return {tabIndex, TypeKind::Subrange};
    }
    else if (typeNode.getKind() == ASTNodeKind::EnumeratedType) {
        std::vector<std::string> values;
        for (auto &&child : typeNode.getChildren()) {
            const ASTNode &enumNode = child.node;
            values.push_back(enumNode.getAttribute("name"));
        }
        int tabIndex = symbolTable_.declareEnumeratedType("_anonymousType" + std::to_string(anonymousTypeCount_++), values);
        ASTAnnotation annotation;
        annotation.typeName = "enumerated";
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);

        return {tabIndex, TypeKind::Enumerated};
    }
    else if (typeNode.getKind() == ASTNodeKind::RecordType) {
        std::string name = "_anonymousType" + std::to_string(anonymousTypeCount_++);
        int recordRef = symbolTable_.beginRecordType(name);
        for (int i = 0; i < (int)typeNode.getChildren().size(); i++) {
            ASTNode &fieldNode = typeNode.childAt(i);
            if (fieldNode.getKind() != ASTNodeKind::FieldDeclaration) continue;
            decorateFieldDeclaration(fieldNode, recordRef);
        }
        symbolTable_.endRecordType();

        ASTAnnotation annotation;
        annotation.typeName = "record";
        annotation.blockIndex = recordRef;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);

        return {recordRef, TypeKind::Record};
    }
    return {0, TypeKind::Unknown};
}
void DecoratedAST::decorateVarDeclaration(ASTNode &node) {
    ASTNode *typeNode = node.childWithRole(ASTChildRole::Type);
    std::string name = node.getAttribute("name");

    if (typeNode->getKind() == ASTNodeKind::NamedType) {
        std::string typeName = typeNode->getAttribute("name");

        const TabEntry &tabEntry = symbolTable_.requireType(typeName);
        int tabIndex = symbolTable_.declareVariable(name, tabEntry.type, tabEntry.ref);
        ASTAnnotation annotation;
        annotation.typeName = typeName;
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
    else if (typeNode->getKind() == ASTNodeKind::ArrayType) {
        const ASTNode *indexNode = typeNode->childWithRole(ASTChildRole::Index);
        ASTNode *elementNode = typeNode->childWithRole(ASTChildRole::Element);

        if (indexNode == nullptr || elementNode == nullptr) {
            throw std::runtime_error("Can't get index or element of array: " + name);
        }

        std::string indexName = indexNode->getAttribute("name");
        TypeKind indexType;
        int indexRef = 0;
        std::string low = "";
        std::string high = "";

        if (indexNode->getKind() == ASTNodeKind::Identifier) {
            const TabEntry &indexTabEntry = symbolTable_.requireType(indexName);
            indexRef = symbolTable_.requireTypeIndex(indexName);

            if (indexTabEntry.type == TypeKind::Subrange) {
                const TypeDescriptor &typeDescriptor = symbolTable_.requireTypeDescriptor(indexTabEntry.ref);
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

        auto [elementRef, elementType] = decorateAnonymousType(*elementNode);
        int arrIndex = symbolTable_.addArrayType(indexType, indexRef, low, high, elementType, elementRef);
        int tabIndex = symbolTable_.declareVariable(name, TypeKind::Array, arrIndex);

        ASTAnnotation annotation;
        annotation.typeName = "array";
        annotation.tabIndex = tabIndex;
        annotation.arrayIndex = arrIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
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
        int subrangeIndex = symbolTable_.declareSubrangeType("_anonymousType" + std::to_string(anonymousTypeCount_++), baseKind, low, high, baseRef);
        int tabIndex = symbolTable_.declareVariable(name, TypeKind::Subrange, subrangeIndex);

        ASTAnnotation annotation;
        annotation.typeName = "subrange";
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
    else if (typeNode->getKind() == ASTNodeKind::EnumeratedType) {
        std::vector<std::string> values;
        for (auto &&child : typeNode->getChildren()) {
            const ASTNode &enumNode = child.node;
            values.push_back(enumNode.getAttribute("name"));
        }
        int enumIndex = symbolTable_.declareEnumeratedType("_anonymousType" + std::to_string(anonymousTypeCount_++), values);
        int tabIndex = symbolTable_.declareVariable(name, TypeKind::Enumerated, enumIndex);

        ASTAnnotation annotation;
        annotation.typeName = "enumerated";
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
    else if (typeNode->getKind() == ASTNodeKind::RecordType) {
        int recordRef = symbolTable_.beginRecordType(name);
        for (int i = 0; i < (int)typeNode->getChildren().size(); i++) {
            ASTNode &fieldNode = typeNode->childAt(i);
            if (fieldNode.getKind() != ASTNodeKind::FieldDeclaration) continue;
            decorateFieldDeclaration(fieldNode, recordRef);
        }
        symbolTable_.endRecordType();
        int tabIndex = symbolTable_.declareVariable(name, TypeKind::Record, recordRef);

        ASTAnnotation annotation;
        annotation.typeName = "record";
        annotation.tabIndex = tabIndex;
        annotation.blockIndex = recordRef;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
}
void DecoratedAST::decorateProcedureDeclaration(ASTNode &node) {
    std::string name = node.getAttribute("name");
    ASTNode *parametersNode = node.childWithRole(ASTChildRole::Parameters);
    ASTNode *blockNode = node.childWithRole(ASTChildRole::Block);

    int tabIndex = symbolTable_.declareProcedureWithBlock(name);
    const TabEntry &tabEntry = symbolTable_.tab().at(tabIndex);
    symbolTable_.enterBlockByIndex(tabEntry.ref);
    dfs(*parametersNode);
    dfs(*blockNode);
    symbolTable_.leaveBlock();

    ASTAnnotation annotation;
    annotation.typeName = "void";
    annotation.tabIndex = tabIndex;
    annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
    node.setAnnotation(annotation);
}
void DecoratedAST::decorateFunctionDeclaration(ASTNode &node) {
    std::string name = node.getAttribute("name");

    ASTNode *returnTypeNode = node.childWithRole(ASTChildRole::ReturnType);
    ASTNode *typeNode = returnTypeNode->childWithRole(ASTChildRole::Type);
    ASTNode *parametersNode = node.childWithRole(ASTChildRole::Parameters);
    ASTNode *blockNode = node.childWithRole(ASTChildRole::Block);

    std::string typeName = typeNode->getAttribute("name");
    auto &returnTypeEntry = symbolTable_.requireType(typeName);
    int returnTypeRef = symbolTable_.requireTypeIndex(typeName);

    int tabIndex = symbolTable_.declareFunctionWithBlock(name, returnTypeEntry.type, returnTypeRef);
    const TabEntry &tabEntry = symbolTable_.tab().at(tabIndex);
    symbolTable_.enterBlockByIndex(tabEntry.ref);
    dfs(*parametersNode);
    dfs(*blockNode);
    symbolTable_.leaveBlock();

    ASTAnnotation annotation;
    annotation.typeName = typeName;
    annotation.tabIndex = tabIndex;
    annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
    node.setAnnotation(annotation);
}
void DecoratedAST::decorateParameter(ASTNode &node) {
    ASTNode *typeNode = node.childWithRole(ASTChildRole::Type);
    std::string name = node.getAttribute("name");

    if (typeNode->getKind() == ASTNodeKind::NamedType) {
        std::string typeName = typeNode->getAttribute("name");

        const TabEntry &tabEntry = symbolTable_.requireType(typeName);
        int tabIndex = symbolTable_.declareParameter(name, tabEntry.type, tabEntry.ref);

        ASTAnnotation annotation;
        annotation.typeName = typeName;
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
    else if (typeNode->getKind() == ASTNodeKind::ArrayType) {
        const ASTNode *indexNode = typeNode->childWithRole(ASTChildRole::Index);
        ASTNode *elementNode = typeNode->childWithRole(ASTChildRole::Element);

        if (indexNode == nullptr || elementNode == nullptr) {
            throw std::runtime_error("Can't get index or element of array: " + name);
        }

        std::string indexName = indexNode->getAttribute("name");
        TypeKind indexType;
        int indexRef = 0;
        std::string low = "";
        std::string high = "";

        if (indexNode->getKind() == ASTNodeKind::Identifier) {
            const TabEntry &indexTabEntry = symbolTable_.requireType(indexName);
            indexRef = symbolTable_.requireTypeIndex(indexName);

            if (indexTabEntry.type == TypeKind::Subrange) {
                const TypeDescriptor &typeDescriptor = symbolTable_.requireTypeDescriptor(indexTabEntry.ref);
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

        auto [elementRef, elementType] = decorateAnonymousType(*elementNode);
        int arrRef = symbolTable_.addArrayType(indexType, indexRef, low, high, elementType, elementRef);
        int paramRef = symbolTable_.declareParameter(name, TypeKind::Array, arrRef);

        ASTAnnotation annotation;
        annotation.typeName = "array";
        annotation.tabIndex = paramRef;
        annotation.arrayIndex = arrRef;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
}
void DecoratedAST::decorateBlock(ASTNode &node, std::string name) {
    int blockIndex = symbolTable_.enterBlock(name);
    for (size_t i = 0; i < node.getChildren().size(); i++) {
        dfs(node.childAt(i));
    }
    symbolTable_.leaveBlock();
    ASTAnnotation annotation;
    annotation.blockIndex = blockIndex;
    annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
    node.setAnnotation(annotation);
}
void DecoratedAST::decorateAssignmentStatement(ASTNode &node) {
    ASTNode *targetNode = node.childWithRole(ASTChildRole::Target);
    std::pair<int, TypeKind> targetType = {0, TypeKind::Unknown};
    if (targetNode->getKind() == ASTNodeKind::Variable) {
        targetType = decorateVariable(*targetNode);
    }
    else if (targetNode->getKind() == ASTNodeKind::ArrayAccess) {
        targetType = decorateArrayAccess(*targetNode);
    }
    else if (targetNode->getKind() == ASTNodeKind::FieldAccess) {
        targetType = decorateFieldAccess(*targetNode);
    }

    ASTNode *valueNode = node.childWithRole(ASTChildRole::Value);
    std::pair<int, TypeKind> valueType = decorateExpression(*valueNode);

    if (!isAssignmentCompatible(targetType.first, targetType.second, valueType.first, valueType.second)) {
        throw std::runtime_error("Incompatible type: " +
                                 symbolTable_.typeKindToString(targetType.second) +
                                 " := " +
                                 symbolTable_.typeKindToString(valueType.second));
    }
}
void DecoratedAST::decorateIfStatement(ASTNode &node) {
    ASTNode *conditionNode = node.childWithRole(ASTChildRole::Condition);
    std::pair<int, TypeKind> valueType = decorateExpression(*conditionNode);
    if (!isTypeCompatible(0, TypeKind::Boolean, valueType.first, valueType.second)) {
        throw std::runtime_error("Condition must be a boolean: " +
                                 symbolTable_.typeKindToString(valueType.second));
    }
    ASTNode *thenNode = node.childWithRole(ASTChildRole::Then);
    ASTNode *elseNode = node.childWithRole(ASTChildRole::Else);
    decorateBlock(*thenNode, "if-then");
    if (elseNode != NULL) {
        decorateBlock(*elseNode, "else");
    }
}
void DecoratedAST::decorateCaseStatement(ASTNode &node) {
    ASTNode *expressionNode = node.childWithRole(ASTChildRole::Expression);
    auto [expressionRef, expressionType] = decorateExpression(*expressionNode);

    for (int i = 0; i < (int)node.getChildren().size(); i++) {
        const ASTChild &child1 = node.getChildren().at(i);
        if (child1.role == ASTChildRole::Branch) {
            ASTNode &branchNode = node.childAt(i);
            for (int j = 0; j < (int)node.getChildren().size(); j++) {
                const ASTChild &child2 = node.getChildren().at(i);
                if (child2.role == ASTChildRole::Label) {
                    ASTNode &labelNode = branchNode.childAt(j);
                    TypeKind labelType = TypeKind::Unknown;
                    if (isLiteralKind(labelNode.getKind())) {
                        labelType = nodeKindLiteralToTypeKind(labelNode.getKind());
                    }
                    else if (labelNode.getKind() == ASTNodeKind::Variable) {
                        std::string typeName = labelNode.getAttribute("name");
                        const TabEntry &tabEntry = symbolTable_.requireLookup(typeName);
                        labelType = tabEntry.type;
                    }
                    if (expressionType != labelType) {
                        throw std::runtime_error("Invalid label type");
                    }
                }
                else if (child2.role == ASTChildRole::Statement) {
                    ASTNode &statementNode = branchNode.childAt(j);
                    decorateNode(statementNode);
                }
            }
        }
    }
}
void DecoratedAST::decorateCaseBranches(ASTNode &node) {}
void DecoratedAST::decorateWhileStatement(ASTNode &node) {
    ASTNode *conditionNode = node.childWithRole(ASTChildRole::Condition);
    std::pair<int, TypeKind> valueType = decorateExpression(*conditionNode);
    if (!isTypeCompatible(0, TypeKind::Boolean, valueType.first, valueType.second)) {
        throw std::runtime_error("Condition must be a boolean: " +
                                 symbolTable_.typeKindToString(valueType.second));
    }
    ASTNode *bodyNode = node.childWithRole(ASTChildRole::Body);
    decorateBlock(*bodyNode, "while");
}
void DecoratedAST::decorateRepeatStatement(ASTNode &node) {
    ASTNode *bodyNode = node.childWithRole(ASTChildRole::Body);
    decorateBlock(*bodyNode, "repeat-until");

    ASTNode *conditionNode = node.childWithRole(ASTChildRole::Condition);
    std::pair<int, TypeKind> valueType = decorateExpression(*conditionNode);
    if (!isTypeCompatible(0, TypeKind::Boolean, valueType.first, valueType.second)) {
        throw std::runtime_error("Condition must be a boolean: " +
                                 symbolTable_.typeKindToString(valueType.second));
    }
}
void DecoratedAST::decorateForStatement(ASTNode &node) {
}
void DecoratedAST::decorateProcedureCall(ASTNode &node) {
    std::string procedureName = node.getAttribute("name");
    int procedureTabIndex = symbolTable_.lookupIndex(procedureName);
    if (procedureTabIndex < 0) {
        throw std::runtime_error("Semantic error: procedure '" + procedureName + "' is not declared");
    }

    const TabEntry &procedureTabEntry = symbolTable_.tab().at(procedureTabIndex);
    if (procedureTabEntry.object != SymbolObjectKind::Procedure &&
        procedureTabEntry.object != SymbolObjectKind::Function) {
        throw std::runtime_error("Semantic error: '" + procedureName + "' is not callable");
    }

    const BTabEntry &procedureBTabEntry = symbolTable_.requireBlock(procedureTabEntry.ref);

    ASTNode *argumentsNode = node.childWithRole(ASTChildRole::Arg);
    int argCount = 0;

    if (isBuiltinWriteLikeProcedure(procedureTabEntry)) {
        if (argumentsNode != nullptr) {
            for (int i = 0; i < (int)argumentsNode->getChildren().size(); i++) {
                const ASTChild &child = argumentsNode->getChildren().at(i);
                if (child.role != ASTChildRole::Arg) continue;

                argCount++;
                ASTNode &argNode = argumentsNode->childAt(i);
                auto [argRef, argType] = decorateExpression(argNode);
                (void)argRef;

                if (!isPrintableType(argType)) {
                    throw std::runtime_error("Semantic error: argument " +
                                             std::to_string(argCount) + " of '" +
                                             procedureName + "' is not printable");
                }
            }
        }

        ASTAnnotation annotation;
        annotation.typeName = symbolTable_.typeKindToString(procedureTabEntry.type);
        annotation.tabIndex = procedureTabIndex;
        annotation.blockIndex = procedureTabEntry.ref;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
        return;
    }

    int expectedArgCount = procedureBTabEntry.lastParameter > procedureTabIndex
                               ? procedureBTabEntry.lastParameter - procedureTabIndex
                               : 0;

    if (argumentsNode != nullptr) {
        for (int i = 0; i < (int)argumentsNode->getChildren().size(); i++) {
            const ASTChild &child = argumentsNode->getChildren().at(i);
            if (child.role == ASTChildRole::Arg) {
                argCount++;
                if (argCount > expectedArgCount) {
                    throw std::runtime_error("Semantic error: procedure '" + procedureName +
                                             "' expects " + std::to_string(expectedArgCount) +
                                             " argument(s), got " + std::to_string(argCount));
                }

                int parTabRef = procedureTabIndex + argCount;
                const TabEntry &parTabEntry = symbolTable_.tab().at(parTabRef);
                ASTNode &argNode = argumentsNode->childAt(i);
                auto [argRef, argType] = decorateExpression(argNode);

                if (!isAssignmentCompatible(parTabEntry.ref, parTabEntry.type, argRef, argType)) {
                    throw std::runtime_error("Semantic error: argument " +
                                             std::to_string(argCount) + " of '" +
                                             procedureName + "' expects " +
                                             symbolTable_.typeKindToString(parTabEntry.type) +
                                             ", got " +
                                             symbolTable_.typeKindToString(argType));
                }
            }
        }
    }
    if (argCount != expectedArgCount) {
        throw std::runtime_error("Semantic error: procedure '" + procedureName +
                                 "' expects " + std::to_string(expectedArgCount) +
                                 " argument(s), got " + std::to_string(argCount));
    }

    ASTAnnotation annotation;
    annotation.typeName = symbolTable_.typeKindToString(procedureTabEntry.type);
    annotation.tabIndex = procedureTabIndex;
    annotation.blockIndex = procedureTabEntry.ref;
    annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
    node.setAnnotation(annotation);
}
std::pair<int, TypeKind> DecoratedAST::decorateFunctionCall(ASTNode &node) {
    std::string functionName = node.getAttribute("name");
    int functionTabIndex = symbolTable_.requireLookupIndex(functionName);
    const TabEntry &functionTabEntry = symbolTable_.requireLookup(functionName);
    const BTabEntry &functionBTabEntry = symbolTable_.requireBlock(functionTabEntry.ref);

    ASTNode *argumentsNode = node.childWithRole(ASTChildRole::Arg);
    int argCount = 0;
    if (argumentsNode != nullptr) {
        for (int i = 0; i < (int)argumentsNode->getChildren().size(); i++) {
            const ASTChild &child = argumentsNode->getChildren().at(i);
            if (child.role == ASTChildRole::Arg) {
                int parTabRef = functionTabIndex + ++argCount;
                if (parTabRef > functionBTabEntry.lastParameter) {
                    throw std::runtime_error("Too many arguments in function call: " + functionName);
                }
                const TabEntry &parTabEntry = symbolTable_.tab().at(parTabRef);
                ASTNode &argNode = argumentsNode->childAt(i);
                auto [argRef, argType] = decorateExpression(argNode);

                if (!isAssignmentCompatible(parTabEntry.ref, parTabEntry.type, argRef, argType)) {
                    throw std::runtime_error("Invalid argument type: " +
                                             std::to_string(parTabEntry.ref) + ":" +
                                             symbolTable_.typeKindToString(parTabEntry.type) +
                                             " <- " +
                                             std::to_string(argRef) + ":" +
                                             symbolTable_.typeKindToString(argType));
                }
            }
        }
    }
    if (functionTabIndex + argCount != functionBTabEntry.lastParameter) {
        throw std::runtime_error("Too few arguments in function call: " + functionName);
    }

    const TabEntry &returnTabEntry = symbolTable_.tab().at(functionBTabEntry.returnRef);
    return {returnTabEntry.ref, returnTabEntry.type};
}
std::pair<int, TypeKind> DecoratedAST::decorateBinaryOperator(ASTNode &node) {
    std::string op = node.getAttribute("operator");
    ASTNode *leftNode = node.childWithRole(ASTChildRole::Left);
    ASTNode *rightNode = node.childWithRole(ASTChildRole::Right);

    std::pair<int, TypeKind> leftType = decorateExpression(*leftNode);
    std::pair<int, TypeKind> rightType = decorateExpression(*rightNode);
    if (!isTypeCompatible(leftType.first, leftType.second, rightType.first, rightType.second)) {
        throw std::runtime_error("Incompatible type in binary operation: " +
                                 symbolTable_.typeKindToString(leftType.second) + " " +
                                 op + " " +
                                 symbolTable_.typeKindToString(rightType.second));
    }

    std::pair<int, TypeKind> type = {0, TypeKind::Unknown};
    if (op == "+") {
        if (isStringType(leftType.second) && isStringType(rightType.second)) {
            type = {0, TypeKind::String};
        }
        else if (isNumberType(leftType.second) && isNumberType(rightType.second)) {
            if (leftType.second == TypeKind::Real || rightType.second == TypeKind::Real) {
                type = {0, TypeKind::Real};
            }
            else {
                type = {0, TypeKind::Integer};
            }
        }
        else {
            throw std::runtime_error("Incompatible type in binary operation: " +
                                     symbolTable_.typeKindToString(leftType.second) + " " +
                                     op + " " +
                                     symbolTable_.typeKindToString(rightType.second));
        }
    }
    else if (op == "-" || op == "*") {
        if (isNumberType(leftType.second) && isNumberType(rightType.second)) {
            if (leftType.second == TypeKind::Real || rightType.second == TypeKind::Real) {
                type = {0, TypeKind::Real};
            }
            else {
                type = {0, TypeKind::Integer};
            }
        }
        else {
            throw std::runtime_error("Incompatible type in binary operation: " +
                                     symbolTable_.typeKindToString(leftType.second) + " " +
                                     op + " " +
                                     symbolTable_.typeKindToString(rightType.second));
        }
    }
    else if (op == "div" || op == "mod") {
        if (leftType.second == TypeKind::Integer || rightType.second == TypeKind::Integer) {
            type = {0, TypeKind::Integer};
        }
        else {
            throw std::runtime_error("Incompatible type in binary operation: " +
                                     symbolTable_.typeKindToString(leftType.second) + " " +
                                     op + " " +
                                     symbolTable_.typeKindToString(rightType.second));
        }
    }
    else if (op == "/") {
        if (isNumberType(leftType.second) && isNumberType(rightType.second)) {
            type = {0, TypeKind::Real};
        }
        else {
            throw std::runtime_error("Incompatible type in binary operation: " +
                                     symbolTable_.typeKindToString(leftType.second) + " " +
                                     op + " " +
                                     symbolTable_.typeKindToString(rightType.second));
        }
    }
    else if (op == "and" || op == "or") {
        if (isBooleanType(leftType.second) && isBooleanType(rightType.second)) {
            type = {0, TypeKind::Boolean};
        }
        else {
            throw std::runtime_error("Incompatible type in binary operation: " +
                                     symbolTable_.typeKindToString(leftType.second) + " " +
                                     op + " " +
                                     symbolTable_.typeKindToString(rightType.second));
        }
    }
    else if (op == "==" || op == "<>") {
        type = {0, TypeKind::Boolean};
    }
    else if (op == ">" || op == ">=" || op == "<" || op == "<=") {
        if (isOrderedType(leftType.second) && isOrderedType(rightType.second)) {
            type = {0, TypeKind::Boolean};
        }
        else {
            throw std::runtime_error("Incompatible type in binary operation: " +
                                     symbolTable_.typeKindToString(leftType.second) + " " +
                                     op + " " +
                                     symbolTable_.typeKindToString(rightType.second));
        }
    }
    else {
        throw std::runtime_error("Unrecognized binary operator: " + op);
    }

    ASTAnnotation annotation;
    annotation.typeName = symbolTable_.typeKindToString(type.second);
    annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
    node.setAnnotation(annotation);

    return type;
}
std::pair<int, TypeKind> DecoratedAST::decorateUnaryOperator(ASTNode &node) {
    std::string op = node.getAttribute("operator");
    ASTNode *expressionNode = node.childWithRole(ASTChildRole::Expression);
    std::pair<int, TypeKind> expressionType = decorateExpression(*expressionNode);

    if (op == "not") {
        if (!isBooleanType(expressionType.second)) {
            throw std::runtime_error("Invalid unary operation: " + op + " " +
                                     symbolTable_.typeKindToString(expressionType.second));
        }
    }
    else if (op == "+") {
        if (!isNumberType(expressionType.second)) {
            throw std::runtime_error("Invalid unary operation: " + op + " " +
                                     symbolTable_.typeKindToString(expressionType.second));
        }
    }
    else if (op == "-") {
        if (!isNumberType(expressionType.second)) {
            throw std::runtime_error("Invalid unary operation: " + op + " " +
                                     symbolTable_.typeKindToString(expressionType.second));
        }
    }
    else {
        throw std::runtime_error("Invalid unary operation: " + op + " " +
                                 symbolTable_.typeKindToString(expressionType.second));
    }

    ASTAnnotation annotation;
    annotation.typeName = symbolTable_.typeKindToString(expressionType.second);
    annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
    node.setAnnotation(annotation);

    return expressionType;
}
std::pair<int, TypeKind> DecoratedAST::decorateVariable(ASTNode &node) {
    std::string name = node.getAttribute("name");
    int varRef = symbolTable_.requireLookupIndex(name);
    const TabEntry &varEntry = symbolTable_.requireLookup(name);

    ASTAnnotation annotation;
    annotation.typeName = symbolTable_.typeKindToString(varEntry.type);
    annotation.tabIndex = varRef;
    annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
    node.setAnnotation(annotation);
    return {varEntry.ref, varEntry.type};
}
std::pair<int, TypeKind> DecoratedAST::decorateArrayAccess(ASTNode &node) {
    ASTNode *varNode = node.childWithRole(ASTChildRole::Base);
    std::string varName = varNode->getAttribute("name");
    int varRef = symbolTable_.requireLookupIndex(varName);
    const TabEntry &varEntry = symbolTable_.requireLookup(varName);

    int arrRef = varEntry.ref;
    const ATabEntry &arrEntry = symbolTable_.requireArray(arrRef);

    for (int i = 0; i < (int)node.getChildren().size(); i++) {
        const ASTChild &child = node.getChildren().at(i);
        if (child.role == ASTChildRole::Index) {
            ASTNode &indexNode = node.childAt(i);
            TypeKind indexType = TypeKind::Unknown;
            if (isLiteralKind(indexNode.getKind())) {
                indexType = nodeKindLiteralToTypeKind(indexNode.getKind());
            }
            else if (indexNode.getKind() == ASTNodeKind::Variable) {
                std::string typeName = indexNode.getAttribute("name");
                const TabEntry &tabEntry = symbolTable_.requireLookup(typeName);
                indexType = tabEntry.type;
            }
            if (indexType != arrEntry.indexType) {
                throw std::runtime_error("Invalid index type on array access");
            }
        }
    }

    ASTAnnotation annotation;
    annotation.typeName = symbolTable_.typeKindToString(arrEntry.elementType);
    annotation.tabIndex = varRef;
    annotation.arrayIndex = arrRef;
    annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
    node.setAnnotation(annotation);
    return {arrEntry.elementRef, arrEntry.elementType};
}
std::pair<int, TypeKind> DecoratedAST::decorateFieldAccess(ASTNode &node) {
    ASTNode *baseNode = node.childWithRole(ASTChildRole::Base);
    if (baseNode->getKind() == ASTNodeKind::FieldAccess) {
        auto [blockRef, fieldType] = decorateFieldAccess(*baseNode);

        symbolTable_.enterBlockByIndex(blockRef);

        std::string fieldName = node.getAttribute("field");
        int fieldRef = symbolTable_.requireLookupIndex(fieldName);
        const TabEntry &fieldEntry = symbolTable_.requireLookup(fieldName);

        symbolTable_.leaveBlock();

        ASTAnnotation annotation;
        annotation.typeName = symbolTable_.typeKindToString(fieldEntry.type);
        annotation.tabIndex = fieldRef;
        annotation.blockIndex = fieldEntry.ref;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
        return {blockRef, fieldEntry.type};
    }
    else if (baseNode->getKind() == ASTNodeKind::Variable) {
        auto [varRef, varType] = decorateVariable(*baseNode);
        symbolTable_.enterBlockByIndex(varRef);

        std::string fieldName = node.getAttribute("field");
        int fieldRef = symbolTable_.requireLookupIndex(fieldName);
        const TabEntry &fieldEntry = symbolTable_.requireLookup(fieldName);

        symbolTable_.leaveBlock();

        ASTAnnotation annotation;
        annotation.typeName = symbolTable_.typeKindToString(fieldEntry.type);
        annotation.tabIndex = fieldRef;
        annotation.blockIndex = fieldEntry.ref;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
        return {fieldEntry.ref, fieldEntry.type};
    }
    return {0, TypeKind::Unknown};
}
std::pair<int, TypeKind> DecoratedAST::decorateExpression(ASTNode &node) {
    std::pair<int, TypeKind> valueType = {0, TypeKind::Unknown};
    if (node.getKind() == ASTNodeKind::BinaryOperation) {
        valueType = decorateBinaryOperator(node);
    }
    else if (node.getKind() == ASTNodeKind::BinaryOperation) {
        valueType = decorateBinaryOperator(node);
    }
    else if (node.getKind() == ASTNodeKind::UnaryOperation) {
        valueType = decorateUnaryOperator(node);
    }
    else if (node.getKind() == ASTNodeKind::FunctionCall) {
        valueType = decorateFunctionCall(node);
    }
    else if (node.getKind() == ASTNodeKind::Variable) {
        valueType = decorateVariable(node);
    }
    else if (node.getKind() == ASTNodeKind::ArrayAccess) {
        valueType = decorateArrayAccess(node);
    }
    else if (node.getKind() == ASTNodeKind::FieldAccess) {
        valueType = decorateFieldAccess(node);
    }
    else {
        valueType.second = nodeKindLiteralToTypeKind(node.getKind());
        valueType.first = symbolTable_.requireTypeIndex(symbolTable_.typeKindToString(valueType.second));
    }
    return valueType;
}

bool DecoratedAST::isAssignmentCompatible(int typeRef1, TypeKind type1, int typeRef2, TypeKind type2) {
    if (type1 == TypeKind::Unknown || type2 == TypeKind::Unknown) return true;
    if (isBooleanType(type1) && isBooleanType(type2)) return true;
    if (isStringType(type1) && isStringType(type2)) return !(type1 == TypeKind::Char && type2 == TypeKind::String);
    if (isNumberType(type1) && isNumberType(type2)) return true;
    if (type1 == TypeKind::Enumerated && type2 == TypeKind::Enumerated) return typeRef1 == typeRef2;
    if (type1 == TypeKind::Array && type2 == TypeKind::Array) {
        if (typeRef1 == typeRef2) return true;
        const ATabEntry &arrEntry1 = symbolTable_.requireArray(typeRef1);
        const ATabEntry &arrEntry2 = symbolTable_.requireArray(typeRef2);
        bool validIndex = arrEntry1.indexType == arrEntry2.indexType &&
                          arrEntry1.lowOrdinal == arrEntry2.lowOrdinal &&
                          arrEntry1.highOrdinal == arrEntry2.highOrdinal;

        const TabEntry &elEntry1 = symbolTable_.tab().at(arrEntry1.elementRef);
        const TabEntry &elEntry2 = symbolTable_.tab().at(arrEntry2.elementRef);
        return validIndex && isAssignmentCompatible(elEntry1.ref, elEntry1.type, elEntry2.ref, elEntry2.type);
    }
    if (type1 == TypeKind::Record && type2 == TypeKind::Record) return typeRef1 == typeRef2;
    if (type1 == TypeKind::Subrange && type2 == TypeKind::Subrange) return typeRef1 == typeRef2;

    return false;

    return false;
}
bool DecoratedAST::isTypeCompatible(int typeRef1, TypeKind type1, int typeRef2, TypeKind type2) {
    if (type1 == TypeKind::Unknown || type2 == TypeKind::Unknown) return true;
    if (isBooleanType(type1) && isBooleanType(type2)) return true;
    if (isStringType(type1) && isStringType(type2)) return true;
    if (isNumberType(type1) && isNumberType(type2)) return true;
    if (type1 == TypeKind::Enumerated && type2 == TypeKind::Enumerated) return typeRef1 == typeRef2;
    if (type1 == TypeKind::Array && type2 == TypeKind::Array) {
        if (typeRef1 == typeRef2) return true;
        const ATabEntry &arrEntry1 = symbolTable_.requireArray(typeRef1);
        const ATabEntry &arrEntry2 = symbolTable_.requireArray(typeRef2);
        bool validIndex = arrEntry1.indexType == arrEntry2.indexType &&
                          arrEntry1.lowOrdinal == arrEntry2.lowOrdinal &&
                          arrEntry1.highOrdinal == arrEntry2.highOrdinal;

        const TabEntry &elEntry1 = symbolTable_.tab().at(arrEntry1.elementRef);
        const TabEntry &elEntry2 = symbolTable_.tab().at(arrEntry2.elementRef);
        return validIndex && isAssignmentCompatible(elEntry1.ref, elEntry1.type, elEntry2.ref, elEntry2.type);
    }
    if (type1 == TypeKind::Record && type2 == TypeKind::Record) return typeRef1 == typeRef2;
    if (type1 == TypeKind::Subrange && type2 == TypeKind::Subrange) return typeRef1 == typeRef2;

    return false;
}

bool DecoratedAST::isOrderedType(TypeKind type) {
    return type == TypeKind::Integer ||
           type == TypeKind::Real ||
           type == TypeKind::Char ||
           type == TypeKind::String;
}
bool DecoratedAST::isNumberType(TypeKind type) {
    return type == TypeKind::Integer ||
           type == TypeKind::Real;
}
bool DecoratedAST::isStringType(TypeKind type) {
    return type == TypeKind::Char ||
           type == TypeKind::String;
}
bool DecoratedAST::isBooleanType(TypeKind type) {
    return type == TypeKind::Boolean;
}
bool DecoratedAST::isBuiltinWriteLikeProcedure(const TabEntry &entry) {
    return entry.object == SymbolObjectKind::Procedure &&
           (entry.identifier == "write" || entry.identifier == "writeln");
}
bool DecoratedAST::isPrintableType(TypeKind type) {
    return isBooleanType(type) ||
           isNumberType(type) ||
           isStringType(type) ||
           type == TypeKind::Subrange ||
           type == TypeKind::Enumerated;
}

static bool hasAnnotation(const ASTAnnotation &annotation) {
    return !annotation.typeName.empty() || annotation.tabIndex != -1 || annotation.arrayIndex != -1 || annotation.blockIndex != -1;
}
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
void DecoratedAST::printTreeHelper(const ASTNode &node, int depth, std::vector<bool> &isLast, std::ostream &out, ASTChildRole role) const {
    for (int i = 0; i < depth; i++) {
        out << ((i == depth - 1)
                    ? (isLast[i] ? "└── " : "├── ")
                    : (isLast[i] ? "    " : "│   "));
    }
    out << ASTNode::kindToString(node.getKind());
    if (role != ASTChildRole::None) {
        out << "[role: " << ASTNode::roleToString(role) << "] ";
    }
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
    auto &annotation = node.annotation();
    if (hasAnnotation(annotation)) {
        bool first = true;
        out << " -> (";
        if (!annotation.typeName.empty()) {
            out << "type: " << annotation.typeName;
            first = false;
        }
        if (annotation.tabIndex != -1) {
            out << (first ? "" : ", ") << "tabIndex: " << annotation.tabIndex;
            first = false;
        }
        if (annotation.arrayIndex != -1) {
            out << (first ? "" : ", ") << "arrayIndex: " << annotation.arrayIndex;
            first = false;
        }
        if (annotation.blockIndex != -1) {
            out << (first ? "" : ", ") << "blockIndex: " << annotation.blockIndex;
            first = false;
        }
        if (annotation.lexicalLevel != -1) {
            out << (first ? "" : ", ") << "lexicalLevel: " << annotation.lexicalLevel;
            first = false;
        }
        out << ")";
    }
    out << "\n";

    auto &children = node.getChildren();
    for (int i = 0; i < (int)children.size(); i++) {
        isLast.push_back(i == ((int)children.size() - 1));
        printTreeHelper(children[i].node, depth + 1, isLast, out, children[i].role);
        isLast.pop_back();
    }
}