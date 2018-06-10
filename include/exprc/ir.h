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

namespace details {

template <typename IdType>
constexpr auto asInt(IdType id) {
    return static_cast<std::underlying_type_t<IdType>>(id);
}

template <typename IdType>
class IdGen {
public:
    IdType operator()() {
        return static_cast<IdType>(m_next++);
    }

private:
    std::underlying_type_t<IdType> m_next{asInt(IdType::FIRST_VALID_ID)};
};

} // namespace details

class Context {
public:

    template <typename Type, typename... Args>
    auto make(Args&&... args) {
        using GenType = details::IdGen<typename Type::Id>;
        return Type{std::get<GenType>(m_next_id)(), std::forward<Args>(args)...};
    }

private:
    std::tuple<details::IdGen<Operand::Id>, details::IdGen<Instruction::Id>> m_next_id;
};

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
    return os << "op<Id:" << details::asInt(op.id) << ">";
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
