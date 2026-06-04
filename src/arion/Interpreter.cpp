#include "Interpreter.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>
#include <vector>

using namespace arion;

namespace {
    enum class ValueKind {
        Uninitialized,
        Integer,
        Real,
        Boolean,
        String
    };

    struct RuntimeValue {
        ValueKind kind = ValueKind::Uninitialized;
        long long integerValue = 0;
        double realValue = 0.0;
        bool booleanValue = false;
        std::string stringValue;

        static RuntimeValue integer(long long value) {
            RuntimeValue runtimeValue;
            runtimeValue.kind = ValueKind::Integer;
            runtimeValue.integerValue = value;
            return runtimeValue;
        }

        static RuntimeValue real(double value) {
            RuntimeValue runtimeValue;
            runtimeValue.kind = ValueKind::Real;
            runtimeValue.realValue = value;
            return runtimeValue;
        }

        static RuntimeValue boolean(bool value) {
            RuntimeValue runtimeValue;
            runtimeValue.kind = ValueKind::Boolean;
            runtimeValue.booleanValue = value;
            return runtimeValue;
        }

        static RuntimeValue string(std::string value) {
            RuntimeValue runtimeValue;
            runtimeValue.kind = ValueKind::String;
            runtimeValue.stringValue = std::move(value);
            return runtimeValue;
        }
    };

    struct FrameInfo {
        std::size_t base = 0;
        std::size_t frameSize = 0;
        std::size_t returnAddress = 0;
    };

    struct IndexAddressOperand {
        int low = 0;
        int high = 0;
        int elementSize = 1;
    };

    std::string lowerCopy(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    std::string trim(const std::string &value) {
        std::size_t begin = 0;
        while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
            ++begin;
        }

        std::size_t end = value.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
            --end;
        }

        return value.substr(begin, end - begin);
    }

    bool isQuoted(const std::string &value, char quote) {
        return value.size() >= 2 && value.front() == quote && value.back() == quote;
    }

    std::string unquote(const std::string &value, char quote) {
        std::string result;
        for (std::size_t i = 1; i + 1 < value.size(); ++i) {
            if (value[i] == quote && i + 1 < value.size() - 1 && value[i + 1] == quote) {
                result.push_back(quote);
                ++i;
            } else {
                result.push_back(value[i]);
            }
        }
        return result;
    }

    RuntimeError runtimeError(std::size_t ip, const std::string &message) {
        return RuntimeError("Runtime error at instruction " + std::to_string(ip) + ": " + message);
    }

    int parseIntOperand(const InstructionLine &instruction, std::size_t ip, const std::string &name) {
        const std::string operand = trim(instruction.operand);
        if (operand.empty()) {
            throw runtimeError(ip, "missing " + name + " operand");
        }

        std::size_t parsed = 0;
        try {
            int value = std::stoi(operand, &parsed);
            if (parsed != operand.size()) {
                throw runtimeError(ip, "invalid " + name + " operand: " + instruction.operand);
            }
            return value;
        } catch (const RuntimeError &) {
            throw;
        } catch (...) {
            throw runtimeError(ip, "invalid " + name + " operand: " + instruction.operand);
        }
    }

    bool isIntegerLiteral(const std::string &value) {
        if (value.empty()) return false;
        std::size_t i = 0;
        if (value[i] == '+' || value[i] == '-') ++i;
        if (i == value.size()) return false;
        for (; i < value.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(value[i]))) return false;
        }
        return true;
    }

    bool isRealLiteral(const std::string &value) {
        bool hasDigit = false;
        bool hasPoint = false;
        std::size_t i = 0;
        if (value.empty()) return false;
        if (value[i] == '+' || value[i] == '-') ++i;
        for (; i < value.size(); ++i) {
            if (std::isdigit(static_cast<unsigned char>(value[i]))) {
                hasDigit = true;
            } else if (value[i] == '.' && !hasPoint) {
                hasPoint = true;
            } else {
                return false;
            }
        }
        return hasDigit && hasPoint;
    }

    RuntimeValue parseLiteral(const std::string &literal) {
        const std::string value = trim(literal);
        const std::string lowered = lowerCopy(value);

        if (lowered == "true") return RuntimeValue::boolean(true);
        if (lowered == "false") return RuntimeValue::boolean(false);
        if (isQuoted(value, '\'')) return RuntimeValue::string(unquote(value, '\''));
        if (isQuoted(value, '"')) return RuntimeValue::string(unquote(value, '"'));

        try {
            if (isIntegerLiteral(value)) return RuntimeValue::integer(std::stoll(value));
            if (isRealLiteral(value)) return RuntimeValue::real(std::stod(value));
        } catch (...) {
            return RuntimeValue::string(value);
        }

        return RuntimeValue::string(value);
    }

    void ensureInitialized(const RuntimeValue &value, std::size_t ip, const std::string &operation) {
        if (value.kind == ValueKind::Uninitialized) {
            throw runtimeError(ip, operation + " used an uninitialized value");
        }
    }

    RuntimeValue popValue(std::vector<RuntimeValue> &stack, std::size_t ip) {
        if (stack.empty()) {
            throw runtimeError(ip, "stack underflow");
        }

        RuntimeValue value = stack.back();
        stack.pop_back();
        ensureInitialized(value, ip, "Stack pop");
        return value;
    }

    bool isNumeric(const RuntimeValue &value) {
        return value.kind == ValueKind::Integer ||
               value.kind == ValueKind::Real ||
               value.kind == ValueKind::Boolean;
    }

    long long asInteger(const RuntimeValue &value, std::size_t ip) {
        ensureInitialized(value, ip, "Integer conversion");
        if (value.kind == ValueKind::Integer) return value.integerValue;
        if (value.kind == ValueKind::Boolean) return value.booleanValue ? 1 : 0;
        if (value.kind == ValueKind::Real) return static_cast<long long>(value.realValue);
        throw runtimeError(ip, "expected numeric value");
    }

    long long asOrdinal(const RuntimeValue &value, std::size_t ip) {
        ensureInitialized(value, ip, "Array index conversion");
        if (value.kind == ValueKind::String) {
            if (value.stringValue.size() == 1) {
                return static_cast<unsigned char>(value.stringValue.front());
            }
            throw runtimeError(ip, "array index string must contain exactly one character");
        }
        return asInteger(value, ip);
    }

    double asReal(const RuntimeValue &value, std::size_t ip) {
        ensureInitialized(value, ip, "Real conversion");
        if (value.kind == ValueKind::Real) return value.realValue;
        if (value.kind == ValueKind::Integer) return static_cast<double>(value.integerValue);
        if (value.kind == ValueKind::Boolean) return value.booleanValue ? 1.0 : 0.0;
        throw runtimeError(ip, "expected numeric value");
    }

    std::string valueToString(const RuntimeValue &value, std::size_t ip) {
        ensureInitialized(value, ip, "String conversion");
        switch (value.kind) {
            case ValueKind::Integer:
                return std::to_string(value.integerValue);
            case ValueKind::Real: {
                std::ostringstream out;
                out << value.realValue;
                return out.str();
            }
            case ValueKind::Boolean:
                return value.booleanValue ? "true" : "false";
            case ValueKind::String:
                return value.stringValue;
            case ValueKind::Uninitialized:
                break;
        }
        throw runtimeError(ip, "uninitialized value");
    }

    bool isTruthy(const RuntimeValue &value, std::size_t ip) {
        ensureInitialized(value, ip, "Conditional jump");
        switch (value.kind) {
            case ValueKind::Boolean:
                return value.booleanValue;
            case ValueKind::Integer:
                return value.integerValue != 0;
            case ValueKind::Real:
                return value.realValue != 0.0;
            case ValueKind::String:
                return !value.stringValue.empty();
            case ValueKind::Uninitialized:
                break;
        }
        throw runtimeError(ip, "uninitialized condition");
    }

    RuntimeValue binaryArithmetic(OperationCode operation, const RuntimeValue &left,
                                  const RuntimeValue &right, std::size_t ip) {
        if (operation == OperationCode::ADD &&
            (left.kind == ValueKind::String || right.kind == ValueKind::String)) {
            return RuntimeValue::string(valueToString(left, ip) + valueToString(right, ip));
        }

        if (!isNumeric(left) || !isNumeric(right)) {
            throw runtimeError(ip, "arithmetic operation expects numeric operands");
        }

        if (operation == OperationCode::MOD) {
            const long long divisor = asInteger(right, ip);
            if (divisor == 0) {
                throw runtimeError(ip, "division by zero in mod operation");
            }
            return RuntimeValue::integer(asInteger(left, ip) % divisor);
        }

        if (left.kind == ValueKind::Real || right.kind == ValueKind::Real) {
            const double leftValue = asReal(left, ip);
            const double rightValue = asReal(right, ip);
            switch (operation) {
                case OperationCode::ADD:
                    return RuntimeValue::real(leftValue + rightValue);
                case OperationCode::SUB:
                    return RuntimeValue::real(leftValue - rightValue);
                case OperationCode::MUL:
                    return RuntimeValue::real(leftValue * rightValue);
                case OperationCode::DIV:
                    if (rightValue == 0.0) {
                        throw runtimeError(ip, "division by zero");
                    }
                    return RuntimeValue::real(leftValue / rightValue);
                default:
                    break;
            }
        } else {
            const long long leftValue = asInteger(left, ip);
            const long long rightValue = asInteger(right, ip);
            switch (operation) {
                case OperationCode::ADD:
                    return RuntimeValue::integer(leftValue + rightValue);
                case OperationCode::SUB:
                    return RuntimeValue::integer(leftValue - rightValue);
                case OperationCode::MUL:
                    return RuntimeValue::integer(leftValue * rightValue);
                case OperationCode::DIV:
                    if (rightValue == 0) {
                        throw runtimeError(ip, "division by zero");
                    }
                    return RuntimeValue::integer(leftValue / rightValue);
                default:
                    break;
            }
        }

        throw runtimeError(ip, "unsupported arithmetic operation");
    }

    int compareValues(const RuntimeValue &left, const RuntimeValue &right, std::size_t ip) {
        if (isNumeric(left) && isNumeric(right)) {
            const double leftValue = asReal(left, ip);
            const double rightValue = asReal(right, ip);
            if (leftValue < rightValue) return -1;
            if (leftValue > rightValue) return 1;
            return 0;
        }

        const std::string leftValue = valueToString(left, ip);
        const std::string rightValue = valueToString(right, ip);
        if (leftValue < rightValue) return -1;
        if (leftValue > rightValue) return 1;
        return 0;
    }

    RuntimeValue binaryComparison(OperationCode operation, const RuntimeValue &left,
                                  const RuntimeValue &right, std::size_t ip) {
        const int comparison = compareValues(left, right, ip);
        switch (operation) {
            case OperationCode::EQL:
                return RuntimeValue::boolean(comparison == 0);
            case OperationCode::NEQ:
                return RuntimeValue::boolean(comparison != 0);
            case OperationCode::LSS:
                return RuntimeValue::boolean(comparison < 0);
            case OperationCode::GEQ:
                return RuntimeValue::boolean(comparison >= 0);
            case OperationCode::GTR:
                return RuntimeValue::boolean(comparison > 0);
            case OperationCode::LEQ:
                return RuntimeValue::boolean(comparison <= 0);
            default:
                break;
        }
        throw runtimeError(ip, "unsupported comparison operation");
    }

    OperationCode operationFromOperand(const InstructionLine &instruction, std::size_t ip) {
        const int operation = parseIntOperand(instruction, ip, "operation");
        switch (operation) {
            case static_cast<int>(OperationCode::NEG):
                return OperationCode::NEG;
            case static_cast<int>(OperationCode::ADD):
                return OperationCode::ADD;
            case static_cast<int>(OperationCode::SUB):
                return OperationCode::SUB;
            case static_cast<int>(OperationCode::MUL):
                return OperationCode::MUL;
            case static_cast<int>(OperationCode::DIV):
                return OperationCode::DIV;
            case static_cast<int>(OperationCode::MOD):
                return OperationCode::MOD;
            case static_cast<int>(OperationCode::EQL):
                return OperationCode::EQL;
            case static_cast<int>(OperationCode::NEQ):
                return OperationCode::NEQ;
            case static_cast<int>(OperationCode::LSS):
                return OperationCode::LSS;
            case static_cast<int>(OperationCode::GEQ):
                return OperationCode::GEQ;
            case static_cast<int>(OperationCode::GTR):
                return OperationCode::GTR;
            case static_cast<int>(OperationCode::LEQ):
                return OperationCode::LEQ;
            case static_cast<int>(OperationCode::WRT):
                return OperationCode::WRT;
            case static_cast<int>(OperationCode::WRTLN):
                return OperationCode::WRTLN;
            default:
                throw runtimeError(ip, "unsupported OPR code: " + std::to_string(operation));
        }
    }

    IndexAddressOperand indexAddressOperandFromInstruction(const InstructionLine &instruction, std::size_t ip) {
        std::istringstream input(instruction.operand);
        IndexAddressOperand operand;
        operand.low = instruction.level;
        if (!(input >> operand.high >> operand.elementSize)) {
            throw runtimeError(ip, "invalid IXA operand: " + instruction.operand);
        }
        std::string extra;
        if (input >> extra) {
            throw runtimeError(ip, "invalid IXA operand: " + instruction.operand);
        }
        if (operand.elementSize <= 0) {
            throw runtimeError(ip, "array element size must be positive");
        }
        return operand;
    }

    std::size_t frameIndexForBase(const std::vector<FrameInfo> &frames, std::size_t base, std::size_t ip) {
        for (std::size_t i = frames.size(); i > 0; --i) {
            if (frames[i - 1].base == base) {
                return i - 1;
            }
        }
        throw runtimeError(ip, "invalid frame base: " + std::to_string(base));
    }

    std::size_t baseForLevel(const std::vector<RuntimeValue> &stack,
                             const std::vector<FrameInfo> &frames,
                             int level,
                             std::size_t ip) {
        if (level < 0) {
            throw runtimeError(ip, "negative lexical level");
        }
        if (frames.empty()) {
            throw runtimeError(ip, "no active stack frame");
        }

        std::size_t base = frames.back().base;
        for (int i = 0; i < level; ++i) {
            if (base >= stack.size()) {
                throw runtimeError(ip, "invalid static link base");
            }
            base = static_cast<std::size_t>(asInteger(stack.at(base), ip));
        }
        frameIndexForBase(frames, base, ip);
        return base;
    }

    void validateAddress(int address, std::size_t base, const std::vector<FrameInfo> &frames, std::size_t ip) {
        const std::size_t frameIndex = frameIndexForBase(frames, base, ip);
        const FrameInfo &frame = frames.at(frameIndex);
        if (address < 0 || static_cast<std::size_t>(address) >= frame.frameSize) {
            throw runtimeError(ip, "invalid address: " + std::to_string(address));
        }
    }

    void validateAbsoluteAddress(long long address, const std::vector<FrameInfo> &frames, std::size_t ip) {
        if (address < 0) {
            throw runtimeError(ip, "invalid indirect address: " + std::to_string(address));
        }

        const std::size_t absoluteAddress = static_cast<std::size_t>(address);
        for (const FrameInfo &frame : frames) {
            if (absoluteAddress >= frame.base &&
                absoluteAddress < frame.base + frame.frameSize) {
                return;
            }
        }
        throw runtimeError(ip, "invalid indirect address: " + std::to_string(address));
    }

    void validateJumpTarget(int target, const IntermediateCode &code, std::size_t ip) {
        if (target < 0 || target >= static_cast<int>(code.getTotalLine())) {
            throw runtimeError(ip, "invalid jump target: " + std::to_string(target));
        }
    }
}

RuntimeError::RuntimeError(const std::string &message)
    : std::runtime_error(message) {}

Interpreter::Interpreter(std::size_t instructionLimit)
    : instructionLimit_(instructionLimit) {}

void Interpreter::execute(const IntermediateCode &code, std::ostream &out) {
    std::vector<RuntimeValue> stack;
    std::vector<FrameInfo> frames;
    std::size_t ip = 0;
    std::size_t executedInstructions = 0;
    bool halted = false;

    while (!halted && ip < code.getTotalLine()) {
        if (executedInstructions++ >= instructionLimit_) {
            throw runtimeError(ip, "instruction limit exceeded");
        }

        const InstructionLine *instruction = code.getInstructionLine(static_cast<int>(ip));
        if (instruction == nullptr) {
            throw runtimeError(ip, "invalid instruction pointer");
        }

        bool jumped = false;
        switch (instruction->opCode) {
            case InstructionOpCode::INT: {
                const int size = parseIntOperand(*instruction, ip, "memory size");
                if (size < 3) {
                    throw runtimeError(ip, "frame size must be at least 3: " + std::to_string(size));
                }

                if (frames.empty()) {
                    frames.push_back(FrameInfo{0, static_cast<std::size_t>(size), code.getTotalLine()});
                    if (stack.size() < 3) {
                        stack.resize(3);
                    }
                    stack[0] = RuntimeValue::integer(0);
                    stack[1] = RuntimeValue::integer(0);
                    stack[2] = RuntimeValue::integer(static_cast<long long>(code.getTotalLine()));
                } else {
                    frames.back().frameSize = static_cast<std::size_t>(size);
                }

                const std::size_t frameEnd = frames.back().base + static_cast<std::size_t>(size);
                if (stack.size() < frameEnd) {
                    stack.resize(frameEnd);
                }
                break;
            }

            case InstructionOpCode::LIT:
                stack.push_back(parseLiteral(instruction->operand));
                break;

            case InstructionOpCode::LOD: {
                const int address = parseIntOperand(*instruction, ip, "address");
                const std::size_t base = baseForLevel(stack, frames, instruction->level, ip);
                validateAddress(address, base, frames, ip);
                const RuntimeValue value = stack.at(base + static_cast<std::size_t>(address));
                ensureInitialized(value, ip, "Load");
                stack.push_back(value);
                break;
            }

            case InstructionOpCode::STO: {
                const int address = parseIntOperand(*instruction, ip, "address");
                const std::size_t base = baseForLevel(stack, frames, instruction->level, ip);
                validateAddress(address, base, frames, ip);
                RuntimeValue value = popValue(stack, ip);
                stack.at(base + static_cast<std::size_t>(address)) = std::move(value);
                break;
            }

            case InstructionOpCode::LDA: {
                const int address = parseIntOperand(*instruction, ip, "address");
                const std::size_t base = baseForLevel(stack, frames, instruction->level, ip);
                validateAddress(address, base, frames, ip);
                stack.push_back(RuntimeValue::integer(static_cast<long long>(base + static_cast<std::size_t>(address))));
                break;
            }

            case InstructionOpCode::IXA: {
                const IndexAddressOperand operand = indexAddressOperandFromInstruction(*instruction, ip);
                const RuntimeValue indexValue = popValue(stack, ip);
                const RuntimeValue baseAddressValue = popValue(stack, ip);
                const long long index = asOrdinal(indexValue, ip);
                if (index < operand.low || index > operand.high) {
                    throw runtimeError(ip, "array index out of bounds: " +
                                           std::to_string(index) + " not in " +
                                           std::to_string(operand.low) + ".." +
                                           std::to_string(operand.high));
                }

                const long long baseAddress = asInteger(baseAddressValue, ip);
                const long long elementAddress = baseAddress + (index - operand.low) * operand.elementSize;
                validateAbsoluteAddress(elementAddress, frames, ip);
                stack.push_back(RuntimeValue::integer(elementAddress));
                break;
            }

            case InstructionOpCode::LDI: {
                const RuntimeValue addressValue = popValue(stack, ip);
                const long long address = asInteger(addressValue, ip);
                validateAbsoluteAddress(address, frames, ip);
                const RuntimeValue value = stack.at(static_cast<std::size_t>(address));
                ensureInitialized(value, ip, "Indirect load");
                stack.push_back(value);
                break;
            }

            case InstructionOpCode::STI: {
                const RuntimeValue addressValue = popValue(stack, ip);
                RuntimeValue value = popValue(stack, ip);
                const long long address = asInteger(addressValue, ip);
                validateAbsoluteAddress(address, frames, ip);
                stack.at(static_cast<std::size_t>(address)) = std::move(value);
                break;
            }

            case InstructionOpCode::JMP: {
                const int target = parseIntOperand(*instruction, ip, "jump target");
                validateJumpTarget(target, code, ip);
                ip = static_cast<std::size_t>(target);
                jumped = true;
                break;
            }

            case InstructionOpCode::JPC: {
                const int target = parseIntOperand(*instruction, ip, "jump target");
                validateJumpTarget(target, code, ip);
                const RuntimeValue condition = popValue(stack, ip);
                if (!isTruthy(condition, ip)) {
                    ip = static_cast<std::size_t>(target);
                    jumped = true;
                }
                break;
            }

            case InstructionOpCode::CAL: {
                const int target = parseIntOperand(*instruction, ip, "call target");
                validateJumpTarget(target, code, ip);

                const RuntimeValue argumentCountValue = popValue(stack, ip);
                const int argumentCount = static_cast<int>(asInteger(argumentCountValue, ip));
                if (argumentCount < 0) {
                    throw runtimeError(ip, "negative argument count");
                }
                if (stack.size() < static_cast<std::size_t>(argumentCount)) {
                    throw runtimeError(ip, "stack underflow while preparing call arguments");
                }

                std::vector<RuntimeValue> arguments;
                const std::size_t firstArgument = stack.size() - static_cast<std::size_t>(argumentCount);
                for (std::size_t i = firstArgument; i < stack.size(); ++i) {
                    arguments.push_back(stack[i]);
                }
                stack.resize(firstArgument);

                const std::size_t staticBase = baseForLevel(stack, frames, instruction->level, ip);
                const std::size_t dynamicBase = frames.empty() ? 0 : frames.back().base;
                const std::size_t newBase = stack.size();
                frames.push_back(FrameInfo{newBase, 3 + arguments.size(), ip + 1});

                stack.push_back(RuntimeValue::integer(static_cast<long long>(staticBase)));
                stack.push_back(RuntimeValue::integer(static_cast<long long>(dynamicBase)));
                stack.push_back(RuntimeValue::integer(static_cast<long long>(ip + 1)));
                for (RuntimeValue &argument : arguments) {
                    stack.push_back(std::move(argument));
                }

                ip = static_cast<std::size_t>(target);
                jumped = true;
                break;
            }

            case InstructionOpCode::OPR: {
                const OperationCode operation = operationFromOperand(*instruction, ip);
                switch (operation) {
                    case OperationCode::NEG: {
                        const RuntimeValue value = popValue(stack, ip);
                        if (!isNumeric(value)) {
                            throw runtimeError(ip, "unary negation expects numeric operand");
                        }
                        if (value.kind == ValueKind::Real) {
                            stack.push_back(RuntimeValue::real(-asReal(value, ip)));
                        } else {
                            stack.push_back(RuntimeValue::integer(-asInteger(value, ip)));
                        }
                        break;
                    }

                    case OperationCode::ADD:
                    case OperationCode::SUB:
                    case OperationCode::MUL:
                    case OperationCode::DIV:
                    case OperationCode::MOD: {
                        const RuntimeValue right = popValue(stack, ip);
                        const RuntimeValue left = popValue(stack, ip);
                        stack.push_back(binaryArithmetic(operation, left, right, ip));
                        break;
                    }

                    case OperationCode::EQL:
                    case OperationCode::NEQ:
                    case OperationCode::LSS:
                    case OperationCode::GEQ:
                    case OperationCode::GTR:
                    case OperationCode::LEQ: {
                        const RuntimeValue right = popValue(stack, ip);
                        const RuntimeValue left = popValue(stack, ip);
                        stack.push_back(binaryComparison(operation, left, right, ip));
                        break;
                    }

                    case OperationCode::WRT: {
                        out << valueToString(popValue(stack, ip), ip);
                        break;
                    }

                    case OperationCode::WRTLN: {
                        out << valueToString(popValue(stack, ip), ip) << '\n';
                        break;
                    }
                }
                break;
            }

            case InstructionOpCode::RET:
                if (frames.empty() || frames.size() == 1) {
                    halted = true;
                } else {
                    FrameInfo frame = frames.back();
                    RuntimeValue returnValue;
                    bool hasReturnValue = false;
                    const std::size_t frameEnd = frame.base + frame.frameSize;
                    if (stack.size() > frameEnd) {
                        returnValue = popValue(stack, ip);
                        hasReturnValue = true;
                    }

                    stack.resize(frame.base);
                    frames.pop_back();
                    if (hasReturnValue) {
                        stack.push_back(std::move(returnValue));
                    }

                    ip = frame.returnAddress;
                    jumped = true;
                }
                break;
        }

        if (!halted && !jumped) {
            ++ip;
        }
    }
}

std::string Interpreter::executeToString(const IntermediateCode &code) {
    std::ostringstream out;
    execute(code, out);
    return out.str();
}
