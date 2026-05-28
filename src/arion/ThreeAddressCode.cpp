#include "ThreeAddressCode.hpp"

#include <iomanip>
#include <sstream>

using namespace arion;

std::string TACLine::operatorToString(TACOperator op) {
    switch (op) {
        case TACOperator::Plus:
            return "+";
        case TACOperator::Minus:
            return "-";
        case TACOperator::Multiply:
            return "*";
        case TACOperator::Divide:
            return "/";
        case TACOperator::IntegerDivide:
            return "div";
        case TACOperator::Mod:
            return "mod";
        case TACOperator::And:
            return "and";
        case TACOperator::Or:
            return "or";
        case TACOperator::Not:
            return "not";
        case TACOperator::Equal:
            return "==";
        case TACOperator::NotEqual:
            return "<>";
        case TACOperator::Less:
            return "<";
        case TACOperator::LessOrEqual:
            return "<=";
        case TACOperator::More:
            return ">";
        case TACOperator::MoreOrEqual:
            return ">=";
        default:
            return "";
    }
}
std::string TACLine::toString() const {
    std::stringstream ss;
    switch (codeType) {
        case TACType::AssignmentWithoutOperator:
            ss << target << " := " << arg1;
            break;

        case TACType::AssignmentWithUnaryOperator:
            ss << target << " := " << operatorToString(op) << " " << arg1;
            break;

        case TACType::AssignmentWithBinaryOperator:
            ss << target << " := " << arg1 << " " << operatorToString(op) << " " << arg2;
            break;

        case TACType::GoToLabel:
            ss << target << ":";
            break;

        case TACType::UnconditionalGoTo:
            ss << "goto " << target;
            break;

        case TACType::ConditionalGoTo:
            ss << "if " << arg1 << " " << operatorToString(op) << " " << arg2 << " then goto " << target;
            break;

        case TACType::Parameter:
            ss << "param " << arg1;
            break;

        case TACType::ArrayRead:
            ss << target << " := " << arg1 << "[" << arg2 << "]";
            break;

        case TACType::ArrayWrite:
            ss << target << "[" << arg1 << "]" << " := " << arg2;
            break;

        case TACType::FunctionCall:
            ss << "call " << arg1 << ", " << arg2;
            break;

        case TACType::ProcedureCall:
            ss << "call " << arg1 << ", " << arg2;
            break;

        case TACType::Return:
            ss << "return";
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

size_t ThreeAddressCode::getTotalLine() const {
    return codeLines_.size();
}
const TACLine *ThreeAddressCode::getCodeLine(int position) const {
    if (position >= 0 && position < (int)codeLines_.size()) {
        return &codeLines_[position];
    }
    else {
        return nullptr;
    }
}
int ThreeAddressCode::getLabelPosition(std::string target) const {
    if (labelPosition_.count(target)) {
        return labelPosition_.at(target);
    }
    else {
        return -1;
    }
}
void ThreeAddressCode::printCode(std::ostream &out) const {
    bool first = true;
    for (auto &&line : codeLines_) {
        if (line.codeType == TACType::GoToLabel && !first) out << "\n";
        out << line.toString() << "\n";
        first = false;
    }
}
std::string ThreeAddressCode::getNextAnonymousVariableName() {
    return "_t" + std::to_string(anonymousVariableCount_++);
}
std::string ThreeAddressCode::getNextAnonymousGoToLabel() {
    return "L" + std::to_string(anonymousGoToCount_++);
}

std::string ThreeAddressCode::makeAssigmentWithoutOperator(std::string arg1, std::string target, std::string comment) {
    codeLines_.push_back(TACLine{
        .op = TACOperator::None,
        .arg1 = arg1,
        .arg2 = "",
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = TACType::AssignmentWithoutOperator});
    return codeLines_.back().target;
}

std::string ThreeAddressCode::makeAssigmentWithUnaryOperator(TACOperator op, std::string arg1, std::string target, std::string comment) {
    codeLines_.push_back(TACLine{
        .op = op,
        .arg1 = arg1,
        .arg2 = "",
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = TACType::AssignmentWithUnaryOperator});
    return codeLines_.back().target;
}

std::string ThreeAddressCode::makeAssigmentWithBinaryOperator(TACOperator op, std::string arg1, std::string arg2, std::string target, std::string comment) {
    codeLines_.push_back(TACLine{
        .op = op,
        .arg1 = arg1,
        .arg2 = arg2,
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = TACType::AssignmentWithBinaryOperator});
    return codeLines_.back().target;
}

std::string ThreeAddressCode::makeGoToLabel(std::string target, std::string comment) {
    codeLines_.push_back(TACLine{
        .op = TACOperator::None,
        .arg1 = "",
        .arg2 = "",
        .target = !target.empty() ? target : getNextAnonymousGoToLabel(),

        .comment = comment,
        .codeType = TACType::GoToLabel});
    return codeLines_.back().target;
}

std::string ThreeAddressCode::makeUnconditionalGoTo(std::string target, std::string comment) {
    codeLines_.push_back(TACLine{
        .op = TACOperator::None,
        .arg1 = "",
        .arg2 = "",
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = TACType::UnconditionalGoTo});
    return codeLines_.back().target;
}

std::string ThreeAddressCode::makeConditionalGoTo(TACOperator op, std::string arg1, std::string arg2, std::string target, std::string comment) {
    codeLines_.push_back(TACLine{
        .op = op,
        .arg1 = arg1,
        .arg2 = arg2,
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = TACType::ConditionalGoTo});
    return codeLines_.back().target;
}

void ThreeAddressCode::setParameter(std::string arg1, std::string comment) {
    codeLines_.push_back(TACLine{
        .op = TACOperator::None,
        .arg1 = arg1,
        .arg2 = "",
        .target = "",

        .comment = comment,
        .codeType = TACType::Parameter});
}

std::string ThreeAddressCode::callFunction(std::string functionName, int argcount, std::string target, std::string comment) {
    if (argcount < 0) throw std::runtime_error("Compiler Fault: argcount can't be negative, got " + std::to_string(argcount));
    codeLines_.push_back(TACLine{
        .op = TACOperator::None,
        .arg1 = functionName,
        .arg2 = std::to_string(argcount),
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = TACType::FunctionCall});
    return codeLines_.back().target;
}

void ThreeAddressCode::callProcedure(std::string procedureName, int argcount, std::string comment) {
    if (argcount < 0) throw std::runtime_error("Compiler Fault: argcount can't be negative, got " + std::to_string(argcount));
    codeLines_.push_back(TACLine{
        .op = TACOperator::None,
        .arg1 = procedureName,
        .arg2 = std::to_string(argcount),
        .target = "",

        .comment = comment,
        .codeType = TACType::ProcedureCall});
}

void ThreeAddressCode::makeReturn(std::string comment) {
    codeLines_.push_back(TACLine{
        .op = TACOperator::None,
        .arg1 = "",
        .arg2 = "",
        .target = "",

        .comment = comment,
        .codeType = TACType::Return});
}

std::string ThreeAddressCode::makeArrayRead(std::string arrayName, std::string index, std::string target, std::string comment) {
    codeLines_.push_back(TACLine{
        .op = TACOperator::None,
        .arg1 = arrayName,
        .arg2 = index,
        .target = !target.empty() ? target : getNextAnonymousVariableName(),

        .comment = comment,
        .codeType = TACType::ArrayRead});
    return codeLines_.back().target;
}

void ThreeAddressCode::makeArrayWrite(std::string arrayName, std::string index, std::string arg1, std::string comment) {
    codeLines_.push_back(TACLine{
        .op = TACOperator::None,
        .arg1 = index,
        .arg2 = arg1,
        .target = arrayName,

        .comment = comment,
        .codeType = TACType::ArrayWrite});
}
