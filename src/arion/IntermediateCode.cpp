#include "IntermediateCode.hpp"

#include <iomanip>
#include <sstream>

using namespace arion;

std::string CodeLine::operatorToString(CodeLineOperator op) {
    switch (op) {
        case CodeLineOperator::Plus:
            return "+";
        case CodeLineOperator::Minus:
            return "-";
        case CodeLineOperator::Multiply:
            return "*";
        case CodeLineOperator::Divide:
            return "/";
        case CodeLineOperator::IntegerDivide:
            return "div";
        case CodeLineOperator::Mod:
            return "mod";
        case CodeLineOperator::And:
            return "and";
        case CodeLineOperator::Or:
            return "or";
        case CodeLineOperator::Not:
            return "not";
        case CodeLineOperator::Equal:
            return "==";
        case CodeLineOperator::NotEqual:
            return "<>";
        case CodeLineOperator::Less:
            return "<";
        case CodeLineOperator::LessOrEqual:
            return "<=";
        case CodeLineOperator::More:
            return ">";
        case CodeLineOperator::MoreOrEqual:
            return ">=";
        default:
            return "";
    }
}
std::string CodeLine::toString() const {
    std::stringstream ss;
    switch (codeType) {
        case CodeLineType::AssignmentWithoutOperator:
            ss << target << " := " << arg1;
            break;

        case CodeLineType::AssignmentWithUnaryOperator:
            ss << target << " := " << operatorToString(op) << " " << arg1;
            break;

        case CodeLineType::AssignmentWithBinaryOperator:
            ss << target << " := " << arg1 << " " << operatorToString(op) << " " << arg2;
            break;

        case CodeLineType::GoToLabel:
            ss << target << ":";
            break;

        case CodeLineType::UnconditionalGoTo:
            ss << "goto " << target;
            break;

        case CodeLineType::ConditionalGoTo:
            ss << "if " << arg1 << " " << operatorToString(op) << " " << arg2 << " then goto " << target;
            break;

        case CodeLineType::Parameter:
            ss << "param " << arg1;
            break;

        case CodeLineType::ArrayRead:
            ss << target << " := " << arg1 << "[" << arg2 << "]";
            break;

        case CodeLineType::ArrayWrite:
            ss << target << "[" << arg1 << "]" << " := " << arg2;
            break;

        default:
            break;
    }

    if (!comment.empty()) {
        int len = ss.str().length();
        if (len < DEFAULT_COMMENT_OFFSET) {
            ss << std::setw(DEFAULT_COMMENT_OFFSET - len) << "";
        }
        ss << " {" << comment << " }";
    }
    return ss.str();
}

IntermediateCode::IntermediateCode(SymbolTable &symbolTable) : symbolTable_{symbolTable} {}

SymbolTable &IntermediateCode::getSymbolTable() const {
    return symbolTable_;
}
size_t IntermediateCode::getTotalLine() const {
    return codeLines_.size();
}
const CodeLine *IntermediateCode::getCodeLine(int position) const {
    if (position >= 0 || position < (int)codeLines_.size()) {
        return &codeLines_[position];
    }
    else {
        return nullptr;
    }
}
int IntermediateCode::getLabelPosition(std::string target) const {
    if (labelPosition_.count(target)) {
        return labelPosition_.at(target);
    }
    else {
        return -1;
    }
}
void IntermediateCode::printCode(std::ostream &out = std::cout) const {
    bool first = true;
    for (auto &&line : codeLines_) {
        if (line.codeType == CodeLineType::GoToLabel && !first) out << "\n";
        out << line.toString() << "\n";
        first = false;
    }
}
std::string IntermediateCode::getNextAnonymousVariableName() {
    return "_t" + std::to_string(anonymousVariableCount_++);
}
std::string IntermediateCode::getNextAnonymousGoToLabel() {
    return "L" + std::to_string(anonymousGoToCount_++);
}

std::string IntermediateCode::makeAssigmentWithoutOperator(std::string arg1, std::string target, std::string comment) {
    codeLines_.push_back(CodeLine{
        .op = CodeLineOperator::None,
        .arg1 = arg1,
        .arg2 = "",
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = CodeLineType::AssignmentWithoutOperator});
    return codeLines_.back().target;
}

std::string IntermediateCode::makeAssigmentWithUnaryOperator(CodeLineOperator op, std::string arg1, std::string target, std::string comment) {
    codeLines_.push_back(CodeLine{
        .op = op,
        .arg1 = arg1,
        .arg2 = "",
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = CodeLineType::AssignmentWithUnaryOperator});
    return codeLines_.back().target;
}

std::string IntermediateCode::makeAssigmentWithBinaryOperator(CodeLineOperator op, std::string arg1, std::string arg2, std::string target, std::string comment) {
    codeLines_.push_back(CodeLine{
        .op = op,
        .arg1 = arg1,
        .arg2 = arg2,
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = CodeLineType::AssignmentWithBinaryOperator});
    return codeLines_.back().target;
}

std::string IntermediateCode::makeGoToLabel(std::string target, std::string comment) {
    codeLines_.push_back(CodeLine{
        .op = CodeLineOperator::None,
        .arg1 = "",
        .arg2 = "",
        .target = !target.empty() ? target : getNextAnonymousGoToLabel(),

        .comment = comment,
        .codeType = CodeLineType::GoToLabel});
    return codeLines_.back().target;
}

std::string IntermediateCode::makeUnconditionalGoTo(std::string target, std::string comment) {
    codeLines_.push_back(CodeLine{
        .op = CodeLineOperator::None,
        .arg1 = "",
        .arg2 = "",
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = CodeLineType::UnconditionalGoTo});
    return codeLines_.back().target;
}

std::string IntermediateCode::makeConditionalGoTo(CodeLineOperator op, std::string arg1, std::string arg2, std::string target, std::string comment) {
    codeLines_.push_back(CodeLine{
        .op = op,
        .arg1 = arg1,
        .arg2 = arg2,
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = CodeLineType::ConditionalGoTo});
    return codeLines_.back().target;
}

void IntermediateCode::setParameter(std::string arg1, std::string comment) {
    codeLines_.push_back(CodeLine{
        .op = CodeLineOperator::None,
        .arg1 = arg1,
        .arg2 = "",
        .target = "",

        .comment = comment,
        .codeType = CodeLineType::Parameter});
}

std::string IntermediateCode::callFunction(std::string functionName, int argcount, std::string target, std::string comment) {
    if (argcount < 0) throw std::runtime_error("Compiler Fault: argcount can't be negative, got " + std::to_string(argcount));
    codeLines_.push_back(CodeLine{
        .op = CodeLineOperator::None,
        .arg1 = functionName,
        .arg2 = std::to_string(argcount),
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = CodeLineType::FunctionCall});
    return codeLines_.back().target;
}

void IntermediateCode::callProcedure(std::string procedureName, int argcount, std::string comment) {
    if (argcount < 0) throw std::runtime_error("Compiler Fault: argcount can't be negative, got " + std::to_string(argcount));
    codeLines_.push_back(CodeLine{
        .op = CodeLineOperator::None,
        .arg1 = procedureName,
        .arg2 = std::to_string(argcount),
        .target = "",

        .comment = comment,
        .codeType = CodeLineType::ProcedureCall});
}

std::string IntermediateCode::makeArrayRead(std::string arrayName, std::string index, std::string target, std::string comment) {
    codeLines_.push_back(CodeLine{
        .op = CodeLineOperator::None,
        .arg1 = arrayName,
        .arg2 = index,
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = CodeLineType::ArrayRead});
    return codeLines_.back().target;
}

void IntermediateCode::makeArrayWrite(std::string arrayName, std::string index, std::string arg1, std::string comment) {
    codeLines_.push_back(CodeLine{
        .op = CodeLineOperator::None,
        .arg1 = index,
        .arg2 = arg1,
        .target = arrayName,

        .comment = comment,
        .codeType = CodeLineType::ArrayWrite});
}
