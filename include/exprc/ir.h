#ifndef EXPRC_IR_H
#define EXPRC_IR_H

#include <cstdint>
#include <list>
#include <type_traits>
#include <tuple>
#include <ostream>
#include <optional>
#include <utility>
#include <vector>

#include <exprc/util.h>

namespace exprc {

enum class Opcode {
    INPUT,
    OUTPUT,
    ADD,
    MUL,
};

struct Operand {
    enum class Id : uint32_t {
        FIRST_VALID_ID = 0,
    };

    operator Id() const {
        return id;
    };

    const Id id;
};

struct Instruction {
    enum class Id : uint32_t {
        FIRST_VALID_ID = 0,
    };

    operator Id() const {
        return id;
    }

    const Id id;
    Opcode opcode;
    std::optional<Operand> dst;
    std::vector<Operand> src;
};

using Context = util::Context<Operand, Instruction>;

inline constexpr auto toStr(Opcode opcode) {
    switch (opcode) {
    case Opcode::INPUT:
        return "INPUT";
    case Opcode::OUTPUT:
        return "OUTPUT";
    case Opcode::ADD:
        return "ADD";
    case Opcode::MUL:
        return "MUL";
    }
    return "???";
}

inline std::ostream& operator<<(std::ostream& os, const Operand& op) {
    return os << "op<Id:" << util::asInt(op.id) << ">";
}

inline std::ostream& operator<<(std::ostream&os, const Instruction& instr) {
    os << "<Id:" << util::asInt(instr.id) << "> ";
    if (instr.dst)
       os << *instr.dst << " = <" << toStr(instr.opcode) << ">";
    for (auto& oper : instr.src)
        os << " " << oper;
    return os;
}

} // namespace exprc

#endif // EXPRC_IR_H
