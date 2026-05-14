#include "AST.hpp"
#include <sstream>
#include <utility>

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

