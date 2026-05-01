#include "CFG.hpp"
#include "Tokenizer.hpp"

using namespace arion;

std::string CFG::Terminal::toString() const {
    return name;
}

std::string CFG::Variable::toString() const {
    return Tokenizer::tokenToString(Token{id, name});
}

void CFG::addVariable(Variable v) {
    if (hasTerminal(v.id)) {
        throw std::runtime_error("Variable IDs must not intersect with Terminal IDs");
    }
    variables_.insert(v);
}
bool CFG::hasVariable(int variableId) const {
    return variables_.count(Variable{variableId, ""});
}
CFG::Variable CFG::getVariable(int variableId) const {
    if (!hasVariable(variableId)) {
        return Variable{-1, "unknown"};
    } else {
        return *variables_.find(Variable{variableId, ""});
    }
}

void CFG::addTerminal(Terminal t) {
    if (hasTerminal(t.id)) {
        throw std::runtime_error("Terminal IDs must not intersect with Variable IDs");
    }
    terminals_.insert(t);
}
bool CFG::hasTerminal(int terminalId) const {
    return terminals_.count(Terminal{terminalId, 0, ""});
}
CFG::Terminal CFG::getTerminal(int terminalId) const {
    if (!hasTerminal(terminalId)) {
        return Terminal{-1, 0, "unknown"};
    } else {
        return *terminals_.find(Terminal{terminalId, 0, ""});
    }
}

void CFG::addProduction(int variableId, std::vector<int> products) {
    if (!hasVariable(variableId)) {
        throw std::runtime_error("Producer must be a valid variable in CFG.");
    }
    for (auto &&pId : products) {
        if (!hasVariable(pId) && !hasTerminal(pId)) {
            throw std::runtime_error("Products must be a valid variable or terminal in CFG.");
        }
    }
    productions_[variableId].push_back(products);
}
const std::vector<std::vector<int>> &CFG::getProductions(int variableId) const {
    if (!hasVariable(variableId)) {
        throw std::runtime_error("Producer must be a valid variable in CFG.");
    }
    return productions_.at(variableId);
}

int CFG::getStartSymbol() {
    return startSymbol_;
}
void CFG::setStartSymbol(int symbolId) {
    if (!hasVariable(symbolId) && !hasTerminal(symbolId)) {
        throw std::runtime_error("Start symbol must be a valid variable or terminal in CFG.");
    }
    startSymbol_ = symbolId;
}