#include "IntermediateCode.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>

using namespace arion;

std::string InstructionLine::opCodeToString(InstructionOpCode opCode) {
    switch (opCode) {
        case InstructionOpCode::LIT:
            return "LIT";
        case InstructionOpCode::LOD:
            return "LOD";
        case InstructionOpCode::STO:
            return "STO";
        case InstructionOpCode::CAL:
            return "CAL";
        case InstructionOpCode::INT:
            return "INT";
        case InstructionOpCode::JMP:
            return "JMP";
        case InstructionOpCode::JPC:
            return "JPC";
        case InstructionOpCode::OPR:
            return "OPR";
        case InstructionOpCode::LDA:
            return "LDA";
        case InstructionOpCode::IXA:
            return "IXA";
        case InstructionOpCode::LDI:
            return "LDI";
        case InstructionOpCode::STI:
            return "STI";
        case InstructionOpCode::RET:
            return "RET";
    }
    return "";
}

int InstructionLine::operationToInt(OperationCode operation) {
    return static_cast<int>(operation);
}

std::string InstructionLine::toString(std::size_t lineNumber) const {
    std::stringstream ss;
    ss << lineNumber << ' ' << opCodeToString(opCode);
    if (!operand.empty() ||
        (opCode != InstructionOpCode::RET &&
         opCode != InstructionOpCode::LDI &&
         opCode != InstructionOpCode::STI)) {
        ss << ' ' << level << ' ' << operand;
    }

    if (!comment.empty()) {
        ss << " { " << comment << " }";
    }
    return ss.str();
}

std::size_t IntermediateCode::getTotalLine() const {
    return instructionLines_.size();
}

const InstructionLine *IntermediateCode::getInstructionLine(int position) const {
    if (position >= 0 && position < static_cast<int>(instructionLines_.size())) {
        return &instructionLines_[position];
    }
    return nullptr;
}

void IntermediateCode::printCode(std::ostream &out) const {
    for (std::size_t i = 0; i < instructionLines_.size(); ++i) {
        out << instructionLines_[i].toString(i) << "\n";
    }
}

std::size_t IntermediateCode::getNextInstructionIndex() const {
    return instructionLines_.size();
}

std::size_t IntermediateCode::emitInstruction(InstructionOpCode opCode, int level, std::string operand, std::string comment) {
    InstructionLine line;
    line.opCode = opCode;
    line.level = level;
    line.operand = std::move(operand);
    line.comment = std::move(comment);
    instructionLines_.push_back(std::move(line));
    return instructionLines_.size() - 1;
}

std::size_t IntermediateCode::emitLIT(std::string value, std::string comment) {
    return emitInstruction(InstructionOpCode::LIT, 0, std::move(value), std::move(comment));
}

std::size_t IntermediateCode::emitLOD(int address, std::string comment) {
    return emitLOD(0, address, std::move(comment));
}

std::size_t IntermediateCode::emitLOD(int level, int address, std::string comment) {
    return emitInstruction(InstructionOpCode::LOD, level, std::to_string(address), std::move(comment));
}

std::size_t IntermediateCode::emitSTO(int address, std::string comment) {
    return emitSTO(0, address, std::move(comment));
}

std::size_t IntermediateCode::emitSTO(int level, int address, std::string comment) {
    return emitInstruction(InstructionOpCode::STO, level, std::to_string(address), std::move(comment));
}

std::size_t IntermediateCode::emitCAL(int line, std::string comment) {
    return emitCAL(0, line, std::move(comment));
}

std::size_t IntermediateCode::emitCAL(int level, int line, std::string comment) {
    return emitInstruction(InstructionOpCode::CAL, level, std::to_string(line), std::move(comment));
}

std::size_t IntermediateCode::emitINT(int size, std::string comment) {
    return emitInstruction(InstructionOpCode::INT, 0, std::to_string(size), std::move(comment));
}

std::size_t IntermediateCode::emitJMP(int line, std::string comment) {
    return emitInstruction(InstructionOpCode::JMP, 0, std::to_string(line), std::move(comment));
}

std::size_t IntermediateCode::emitJPC(int line, std::string comment) {
    return emitInstruction(InstructionOpCode::JPC, 0, std::to_string(line), std::move(comment));
}

std::size_t IntermediateCode::emitOPR(OperationCode operation, std::string comment) {
    return emitInstruction(InstructionOpCode::OPR, 0, std::to_string(InstructionLine::operationToInt(operation)), std::move(comment));
}

std::size_t IntermediateCode::emitLDA(int level, int address, std::string comment) {
    return emitInstruction(InstructionOpCode::LDA, level, std::to_string(address), std::move(comment));
}

std::size_t IntermediateCode::emitIXA(int low, int high, int elementSize, std::string comment) {
    return emitInstruction(InstructionOpCode::IXA, low,
                           std::to_string(high) + " " +
                           std::to_string(elementSize),
                           std::move(comment));
}

std::size_t IntermediateCode::emitLDI(std::string comment) {
    return emitInstruction(InstructionOpCode::LDI, 0, "", std::move(comment));
}

std::size_t IntermediateCode::emitSTI(std::string comment) {
    return emitInstruction(InstructionOpCode::STI, 0, "", std::move(comment));
}

std::size_t IntermediateCode::emitRET(std::string comment) {
    return emitInstruction(InstructionOpCode::RET, 0, "", std::move(comment));
}

void IntermediateCode::patchInstructionOperand(std::size_t line, int operand) {
    if (line >= instructionLines_.size()) {
        throw std::out_of_range("Invalid intermediate instruction line: " + std::to_string(line));
    }
    instructionLines_[line].operand = std::to_string(operand);
}
