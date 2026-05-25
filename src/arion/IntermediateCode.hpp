#pragma once

#include "SymbolTable.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace arion
{
    enum class CodeLineType {
        GoToLabel,
        ConditionalGoTo,
        UnconditionalGoTo,
        AssignmentWithoutOperator,
        AssignmentWithBinaryOperator,
        AssignmentWithUnaryOperator,
        Parameter,
        FunctionCall,
        ProcedureCall,
        ArrayRead,
        ArrayWrite
    };

    enum class CodeLineOperator {
        Plus,
        Minus,
        Multiply,
        Divide,
        IntegerDivide,
        Mod,
        And,
        Or,
        Not,
        Equal,
        NotEqual,
        Less,
        LessOrEqual,
        More,
        MoreOrEqual,
        None
    };

    class CodeLine {
    public:
        CodeLineOperator op;
        std::string arg1;
        std::string arg2;
        std::string target;

        std::string comment;

        CodeLineType codeType;

        std::string toString() const;
        static std::string operatorToString(CodeLineOperator op);
        static const int DEFAULT_COMMENT_OFFSET = 24;
    };

    class IntermediateCode {
    private:
        std::vector<CodeLine> codeLines_;
        std::unordered_map<std::string, int> labelPosition_;
        
        int anonymousVariableCount_ = 0;
        int anonymousGoToCount_ = 0;
        std::string getNextAnonymousVariableName();
        std::string getNextAnonymousGoToLabel();

    public:
        size_t getTotalLine() const;
        const CodeLine *getCodeLine(int position) const;
        int getLabelPosition(std::string target) const;
        void printCode(std::ostream &out = std::cout) const;

        // target := arg1 { comment }
        std::string makeAssigmentWithoutOperator(std::string arg1, std::string target = "", std::string comment = "");

        // target := op arg1 { comment }
        std::string makeAssigmentWithUnaryOperator(CodeLineOperator op, std::string arg1, std::string target = "", std::string comment = "");

        // target := arg1 op arg2 { comment }
        std::string makeAssigmentWithBinaryOperator(CodeLineOperator op, std::string arg1, std::string arg2, std::string target = "", std::string comment = "");

        // target: { comment }
        std::string makeGoToLabel(std::string target = "", std::string comment = "");

        // goto target { comment }
        std::string makeUnconditionalGoTo(std::string target = "", std::string comment = "");

        // if arg1 op arg2 then goto target { comment }
        std::string makeConditionalGoTo(CodeLineOperator op, std::string arg1, std::string arg2, std::string target = "", std::string comment = "");

        // Panggil ini beberapa kali untuk memasukkan nilai argument
        // param arg1 { comment }
        void setParameter(std::string arg1, std::string comment = "");

        // Contoh pemakaian:
        // setParameter(arg1); setParameter(arg2); setParameter(arg3);
        // callFunction(target, name, 3);
        // Hasil:
        // target := call functionName(arg1, arg2, arg3) { comment }
        std::string callFunction(std::string functionName, int argcount, std::string target = "", std::string comment = "");

        // Contoh pemakaian:
        // setParameter(arg1); setParameter(arg2); setParameter(arg3);
        // callProcedure(target, name, 3);
        // Hasil:
        // call procedureName(arg1, arg2, arg3) { comment }
        void callProcedure(std::string procedureName, int argcount, std::string comment = "");

        // target := arrayName[index]
        std::string makeArrayRead(std::string arrayName, std::string index, std::string target = "", std::string comment = "");

        // arrayName[index] := arg1
        void makeArrayWrite(std::string arrayName, std::string index, std::string arg1, std::string comment = "");
    };
}