#include "AST.hpp"
#include <iostream>

using namespace arion;

ASTNode::ASTNode(ASTNodeKind kind) : kind_(kind) {}

ASTNode &ASTNode::addChild(ASTNode child) {
    return addChild(ASTChildRole::None, std::move(child));
}

ASTNode &ASTNode::addChild(ASTChildRole role, ASTNode child) {
    children_.push_back({std::move(role), std::move(child)});
    return children_.back().node;
}

ASTNode &ASTNode::setAttribute(std::string key, std::string value) {
    for (auto &attribute : attributes_) {
        if (attribute.first == key) {
            attribute.second = std::move(value);
            return *this;
        }
    }
    attributes_.push_back({std::move(key), std::move(value)});
    return *this;
}

ASTNodeKind ASTNode::getKind() const {
    return kind_;
}

std::string ASTNode::getAttribute(const std::string &key) const {
    for (const auto &attribute : attributes_) {
        if (attribute.first == key) {
            return attribute.second;
        }
    }
    return "";
}

const std::vector<ASTChild> &ASTNode::getChildren() const {
    return children_;
}

const std::vector<std::pair<std::string, std::string>> &ASTNode::getAttributes() const {
    return attributes_;
}

const ASTNode &ASTNode::childAt(std::size_t index) const {
    return children_.at(index).node;
}

ASTNode &ASTNode::childAt(std::size_t index) {
    return children_.at(index).node;
}

const ASTNode *ASTNode::childWithRole(ASTChildRole role) const {
    for (const ASTChild &child : children_) {
        if (child.role == role) {
            return &child.node;
        }
    }
    return nullptr;
}

void ASTNode::setAnnotation(ASTAnnotation annotation) {
    annotation_ = std::move(annotation);
}

ASTAnnotation &ASTNode::annotation() {
    return annotation_;
}

const ASTAnnotation &ASTNode::annotation() const {
    return annotation_;
}

std::string ASTNode::kindToString(ASTNodeKind kind) {
    switch (kind) {
        case ASTNodeKind::Program:
            return "Program";
        case ASTNodeKind::Declarations:
            return "Declarations";
        case ASTNodeKind::ConstDeclarations:
            return "ConstDeclarations";
        case ASTNodeKind::ConstDeclaration:
            return "ConstDeclaration";
        case ASTNodeKind::TypeDeclarations:
            return "TypeDeclarations";
        case ASTNodeKind::TypeDeclaration:
            return "TypeDeclaration";
        case ASTNodeKind::VarDeclarations:
            return "VarDeclarations";
        case ASTNodeKind::VarDeclaration:
            return "VarDeclaration";
        case ASTNodeKind::FieldDeclaration:
            return "FieldDeclaration";
        case ASTNodeKind::ProcedureDeclaration:
            return "ProcedureDeclaration";
        case ASTNodeKind::FunctionDeclaration:
            return "FunctionDeclaration";
        case ASTNodeKind::Parameters:
            return "Parameters";
        case ASTNodeKind::ParameterGroup:
            return "ParameterGroup";
        case ASTNodeKind::Parameter:
            return "Parameter";
        case ASTNodeKind::Block:
            return "Block";
        case ASTNodeKind::CompoundStatement:
            return "CompoundStatement";
        case ASTNodeKind::StatementList:
            return "StatementList";
        case ASTNodeKind::EmptyStatement:
            return "EmptyStatement";
        case ASTNodeKind::Assignment:
            return "Assignment";
        case ASTNodeKind::IfStatement:
            return "IfStatement";
        case ASTNodeKind::CaseStatement:
            return "CaseStatement";
        case ASTNodeKind::CaseBranch:
            return "CaseBranch";
        case ASTNodeKind::WhileStatement:
            return "WhileStatement";
        case ASTNodeKind::RepeatStatement:
            return "RepeatStatement";
        case ASTNodeKind::ForStatement:
            return "ForStatement";
        case ASTNodeKind::ProcedureCall:
            return "ProcedureCall";
        case ASTNodeKind::FunctionCall:
            return "FunctionCall";
        case ASTNodeKind::Arguments:
            return "Arguments";
        case ASTNodeKind::BinaryOperation:
            return "BinaryOperation";
        case ASTNodeKind::UnaryOperation:
            return "UnaryOperation";
        case ASTNodeKind::Variable:
            return "Variable";
        case ASTNodeKind::ArrayAccess:
            return "ArrayAccess";
        case ASTNodeKind::FieldAccess:
            return "FieldAccess";
        case ASTNodeKind::IntegerLiteral:
            return "IntegerLiteral";
        case ASTNodeKind::RealLiteral:
            return "RealLiteral";
        case ASTNodeKind::CharLiteral:
            return "CharLiteral";
        case ASTNodeKind::StringLiteral:
            return "StringLiteral";
        case ASTNodeKind::BooleanLiteral:
            return "BooleanLiteral";
        case ASTNodeKind::NamedType:
            return "NamedType";
        case ASTNodeKind::ReturnType:
            return "ReturnType";
        case ASTNodeKind::ArrayType:
            return "ArrayType";
        case ASTNodeKind::RecordType:
            return "RecordType";
        case ASTNodeKind::RangeType:
            return "RangeType";
        case ASTNodeKind::EnumeratedType:
            return "EnumeratedType";
        case ASTNodeKind::Identifier:
            return "Identifier";
        case ASTNodeKind::Unknown:
        default:
            return "Unknown";
    }
}

std::string ASTNode::roleToString(ASTChildRole role) {
    switch (role) {
        case ASTChildRole::None:
            return "None";
        case ASTChildRole::Declaration:
            return "Declaration";
        case ASTChildRole::Const:
            return "Const";
        case ASTChildRole::Type:
            return "Type";
        case ASTChildRole::Variable:
            return "Variable";
        case ASTChildRole::Field:
            return "Field";
        case ASTChildRole::Parameter:
            return "Parameter";
        case ASTChildRole::Group:
            return "Group";
        case ASTChildRole::Parameters:
            return "Parameters";
        case ASTChildRole::ReturnType:
            return "ReturnType";
        case ASTChildRole::Block:
            return "Block";
        case ASTChildRole::Body:
            return "Body";
        case ASTChildRole::Statement:
            return "Statement";
        case ASTChildRole::Target:
            return "Target";
        case ASTChildRole::Value:
            return "Value";
        case ASTChildRole::Condition:
            return "Condition";
        case ASTChildRole::Then:
            return "Then";
        case ASTChildRole::Else:
            return "Else";
        case ASTChildRole::Expression:
            return "Expression";
        case ASTChildRole::Branch:
            return "Branch";
        case ASTChildRole::Label:
            return "Label";
        case ASTChildRole::Left:
            return "Left";
        case ASTChildRole::Right:
            return "Right";
        case ASTChildRole::Base:
            return "Base";
        case ASTChildRole::Index:
            return "Index";
        case ASTChildRole::Element:
            return "Element";
        case ASTChildRole::Low:
            return "Low";
        case ASTChildRole::High:
            return "High";
        case ASTChildRole::Arg:
            return "Arg";
        case ASTChildRole::Start:
            return "Start";
        case ASTChildRole::End:
            return "End";
    }
    return "";
}

void ASTNode::printTree(std::ostream &out) const {
    std::vector<bool> isLast = {};
    out << kindToString(kind_);
    if (attributes_.size() > 0) {
        out << " (";
        for (int i = 0; i < (int)attributes_.size(); i++) {
            out << attributes_[i].first << ": " << attributes_[i].second;
            if (i == (int)attributes_.size() - 1) {
                out << ")";
            }
            else {
                out << ", ";
            }
        }
    }
    out << "\n";
    
    for (int i = 0; i < (int)children_.size(); i++) {
        isLast.push_back(i == ((int)children_.size() - 1));
        children_[i].node.printTreeHelper(1, isLast, children_[i].role, out);
        isLast.pop_back();
    }
}

void ASTNode::printTreeHelper(int depth, std::vector<bool> &isLast, ASTChildRole role, std::ostream &out) const {
    for (int i = 0; i < depth; i++) {
        out << ((i == depth - 1)
                    ? (isLast[i] ? "└── " : "├── ")
                    : (isLast[i] ? "    " : "│   "));
    }
    out << kindToString(kind_) << "[role: " << roleToString(role) << "] ";
    if (attributes_.size() > 0) {
        out << "(";
        for (int i = 0; i < (int)attributes_.size(); i++) {
            out << attributes_[i].first << ": " << attributes_[i].second;
            if (i == (int)attributes_.size() - 1) {
                out << ")";
            }
            else {
                out << ", ";
            }
        }
    }
    out << "\n";
    for (int i = 0; i < (int)children_.size(); i++) {
        isLast.push_back(i == ((int)children_.size() - 1));
        children_[i].node.printTreeHelper(depth + 1, isLast, children_[i].role, out);
        isLast.pop_back();
    }
}
