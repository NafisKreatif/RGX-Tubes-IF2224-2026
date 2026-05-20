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

        case ASTNodeKind::CompoundStatement:
            decorateCompoundStatement(astNode);
            return true;

        case ASTNodeKind::StatementList:
            decorateStatementList(astNode);
            return true;

        case ASTNodeKind::EmptyStatement:
            // decorateEmptyStatement(astNode);
            return true;

        case ASTNodeKind::Assignment:
            // decorateAssignment(astNode);
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
            // decorateProcedureCall(astNode);
            return true;

        case ASTNodeKind::FunctionCall:
            // decorateFunctionCall(astNode);
            return true;

        case ASTNodeKind::Arguments:
            // decorateArguments(astNode);
            return true;

        case ASTNodeKind::BinaryOperation:
            // decorateBinaryOperation(astNode);
            return true;

        case ASTNodeKind::UnaryOperation:
            // decorateUnaryOperation(astNode);
            return true;

        case ASTNodeKind::Variable:
            decorateVariable(astNode);
            return true;

        case ASTNodeKind::ArrayAccess:
            // decorateArrayAccess(astNode);
            return true;

        case ASTNodeKind::FieldAccess:
            // decorateFieldAccess(astNode);
            return true;

        case ASTNodeKind::IntegerLiteral:
            // decorateIntegerLiteral(astNode);
            return true;

        case ASTNodeKind::RealLiteral:
            // decorateRealLiteral(astNode);
            return true;

        case ASTNodeKind::CharLiteral:
            // decorateCharLiteral(astNode);
            return true;

        case ASTNodeKind::StringLiteral:
            // decorateStringLiteral(astNode);
            return true;

        case ASTNodeKind::BooleanLiteral:
            // decorateBooleanLiteral(astNode);
            return true;

        case ASTNodeKind::NamedType:
            // decorateNamedType(astNode);
            return true;

        case ASTNodeKind::ReturnType:
            // decorateReturnType(astNode);
            return true;

        case ASTNodeKind::ArrayType:
            return true;

        case ASTNodeKind::RecordType:
            return true;

        case ASTNodeKind::RangeType:
            // decorateRangeType(astNode);
            return true;

        case ASTNodeKind::EnumeratedType:
            // decorateEnumeratedType(astNode);
            return true;

        case ASTNodeKind::Identifier:
            // decorateIdentifier(astNode);
            return true;

        case ASTNodeKind::Unknown:
            // decorateUnknown(astNode);
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
        int ref = symbolTable_.requireTypeIndex(typeName);
        int tabIndex = symbolTable_.declareType(name, tabEntry.type, ref);
        ASTAnnotation annotation;
        annotation.typeName = typeName;
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
    else if (typeNode->getKind() == ASTNodeKind::ArrayType) {
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

        std::string elementName = elementNode->getAttribute("name");
        const TabEntry &elementTabEntry = symbolTable_.requireType(elementName);
        int elementRef = symbolTable_.requireTypeIndex(elementName);
        int tabIndex = symbolTable_.declareArrayType(name, indexType, indexRef, low, high, elementTabEntry.type, elementRef);

        ASTAnnotation annotation;
        annotation.typeName = "array";
        annotation.tabIndex = tabIndex;
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
        for (int i = 0; i < typeNode->getChildren().size(); i++) {
            ASTNode &fieldNode = typeNode->childAt(i);
            ASTNode *fieldTypeNode = fieldNode.childWithRole(ASTChildRole::Type);

            std::string fieldName = fieldNode.getAttribute("name");
            std::string fieldTypeName = fieldTypeNode->getAttribute("name");
            const TabEntry &fieldTypeEntry = symbolTable_.requireType(fieldTypeName);
            int fieldTypeRef = symbolTable_.requireTypeIndex(fieldTypeName);
            int tabIndex = symbolTable_.declareField(fieldName, fieldTypeEntry.type, fieldTypeRef);
            ASTAnnotation annotation;
            annotation.typeName = "field";
            annotation.tabIndex = tabIndex;
            annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
            fieldNode.setAnnotation(annotation);
        }
        symbolTable_.endRecordType();
        int recordTabIndex = symbolTable_.declareRecordType(name, recordRef);

        ASTAnnotation annotation;
        annotation.typeName = "record";
        annotation.tabIndex = recordTabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
}

void DecoratedAST::decorateVarDeclaration(ASTNode &node) {
    const ASTNode *typeNode = node.childWithRole(ASTChildRole::Type);
    std::string name = node.getAttribute("name");

    const TabEntry &typeEntry = symbolTable_.requireType(typeNode->getAttribute("name"));
    int typeRef = symbolTable_.requireTypeIndex(typeNode->getAttribute("name"));
    int tabIndex = symbolTable_.declareVariable(name, typeEntry.type, typeRef);

    ASTAnnotation annotation;
    annotation.typeName = typeEntry.identifier;
    annotation.tabIndex = tabIndex;
    annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
    node.setAnnotation(annotation);
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

void DecoratedAST::decorateBlock(ASTNode &node) {}
void DecoratedAST::decorateFormalParameterList(ASTNode &node) {}
void DecoratedAST::decorateParameter(ASTNode &node) {
    const ASTNode *typeNode = node.childWithRole(ASTChildRole::Type);
    std::string name = node.getAttribute("name");

    if (typeNode->getKind() == ASTNodeKind::NamedType) {
        std::string typeName = typeNode->getAttribute("name");

        const TabEntry &tabEntry = symbolTable_.requireType(typeName);
        int ref = symbolTable_.requireTypeIndex(typeName);
        int tabIndex = symbolTable_.declareType(name, tabEntry.type, ref);

        ASTAnnotation annotation;
        annotation.typeName = typeName;
        annotation.tabIndex = tabIndex;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
    else if (typeNode->getKind() == ASTNodeKind::ArrayType) {
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

        std::string elementName = elementNode->getAttribute("name");
        const TabEntry &elementTabEntry = symbolTable_.requireType(elementName);
        int elementRef = symbolTable_.requireTypeIndex(elementName);
        int typeRef = symbolTable_.declareArrayType("arrparam" + std::to_string(array_parameter_count++),
                                                    indexType, indexRef, low, high, elementTabEntry.type, elementRef);

        int paramRef = symbolTable_.declareParameter(name, TypeKind::Array, typeRef);

        ASTAnnotation annotation;
        annotation.typeName = "array";
        annotation.tabIndex = paramRef;
        annotation.lexicalLevel = symbolTable_.currentLexicalLevel();
        node.setAnnotation(annotation);
    }
}
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

static bool hasAnnotation(const ASTAnnotation &annotation) {
    return !annotation.typeName.empty() || annotation.tabIndex != -1 || annotation.arrayIndex != -1 || annotation.blockIndex != -1;
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
        printTreeHelper(children[i].node, depth + 1, isLast, out);
        isLast.pop_back();
    }
}