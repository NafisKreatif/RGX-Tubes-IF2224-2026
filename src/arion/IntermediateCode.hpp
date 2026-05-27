#ifndef ARION_INTERMEDIATE_CODE_H
#define ARION_INTERMEDIATE_CODE_H

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace arion {
    enum class InstructionOpCode {
        LIT,
        LOD,
        STO,
        CAL,
        INT,
        JMP,
        JPC,
        OPR,
        RET
    };

    enum class OperationCode {
        NEG = 1,
        ADD = 2,
        SUB = 3,
        MUL = 4,
        DIV = 5,
        MOD = 6,
        EQL = 7,
        NEQ = 8,
        LSS = 9,
        GEQ = 10,
        GTR = 11,
        LEQ = 12,
        WRT = 13,
        WRTLN = 14
    };

    class InstructionLine {
    public:
        InstructionOpCode opCode = InstructionOpCode::LIT;
        int level = 0;
        std::string operand;
        std::string comment;

        std::string toString(std::size_t lineNumber) const;
        static std::string opCodeToString(InstructionOpCode opCode);
        static int operationToInt(OperationCode operation);
    };

    class IntermediateCode {
    private:
        std::vector<InstructionLine> instructionLines_;

    public:
        std::size_t getTotalLine() const;
        const InstructionLine *getInstructionLine(int position) const;
        void printCode(std::ostream &out = std::cout) const;

        std::size_t getNextInstructionIndex() const;
        std::size_t emitInstruction(InstructionOpCode opCode, int level = 0, std::string operand = "", std::string comment = "");
        std::size_t emitLIT(std::string value, std::string comment = "");
        std::size_t emitLOD(int address, std::string comment = "");
        std::size_t emitSTO(int address, std::string comment = "");
        std::size_t emitCAL(int line, std::string comment = "");
        std::size_t emitINT(int size, std::string comment = "");
        std::size_t emitJMP(int line, std::string comment = "");
        std::size_t emitJPC(int line, std::string comment = "");
        std::size_t emitOPR(OperationCode operation, std::string comment = "");
        std::size_t emitRET(std::string comment = "");
        void patchInstructionOperand(std::size_t line, int operand);
    };
}

#endif
