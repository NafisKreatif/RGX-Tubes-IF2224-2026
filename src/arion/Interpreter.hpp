#ifndef ARION_INTERPRETER_H
#define ARION_INTERPRETER_H

#include "IntermediateCode.hpp"

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

namespace arion {
    class RuntimeError : public std::runtime_error {
    public:
        explicit RuntimeError(const std::string &message);
    };

    class Interpreter {
    public:
        explicit Interpreter(std::size_t instructionLimit = 1000000);

        void execute(const IntermediateCode &code, std::ostream &out = std::cout);
        std::string executeToString(const IntermediateCode &code);

    private:
        std::size_t instructionLimit_;
    };
}

#endif
