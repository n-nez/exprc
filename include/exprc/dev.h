#ifndef EXPRC_DEV_H
#define EXPRC_DEV_H

#include <array>
#include <ostream>
#include <string>
#include <variant>

#include <exprc/ir.h>

namespace exprc {

namespace dev {

struct InPort {
    enum class Id : uint32_t {
        FIRST_VALID_ID = 0,
    };

    operator Id() const {
        return id;
    }

    const Id id;
};

struct OutPort {
    enum class Id : uint32_t {
        FIRST_VALID_ID = 0,
    };

    operator Id() const {
        return id;
    }

    const Id id;
};

enum class DeviceId : uint32_t {
    FIRST_VALID_ID = 0,
};

struct Input {
    operator DeviceId() const {
        return id;
    }

    const DeviceId id;

    const std::string name;
    OutPort out;
    std::array<InPort, 0> in;
};

struct Output {
    operator DeviceId() const {
        return id;
    }

    const DeviceId id;

    const std::string name;
    std::array<InPort, 1> in;
};

struct Adder {
    operator DeviceId() const {
        return id;
    }

    const DeviceId id;

    OutPort out;
    std::array<InPort, 2> in;
};

struct Multiplier {
    operator DeviceId() const {
        return id;
    }

    const DeviceId id;

    OutPort out;
    std::array<InPort, 2> in;
};

struct Register {
    operator DeviceId() const {
        return id;
    }

    const DeviceId id;

    OutPort out;
    std::array<InPort, 1> in;
};

using Device = std::variant<InPort, OutPort, Input, Adder, Multiplier, Register>;

class Context {
public:
    template <typename T>
    T make();

    template <typename T>
    T make(const std::string&);

private:
    util::IdGen<InPort::Id> m_next_in_id;
    util::IdGen<OutPort::Id> m_next_out_id;
    util::IdGen<DeviceId> m_next_id;
};

template <>
inline InPort Context::make<InPort>() {
    return InPort{m_next_in_id()};
}

template <>
inline OutPort Context::make<OutPort>() {
    return OutPort{m_next_out_id()};
}

template <>
inline Input Context::make<Input>(const std::string& name) {
    return Input{m_next_id(), name, make<OutPort>()};
}

template <>
inline Output Context::make<Output>(const std::string& name) {
    return Output{m_next_id(), name, {make<InPort>()}};
}

template <>
inline Adder Context::make<Adder>() {
    return Adder{m_next_id(), make<OutPort>(), {make<InPort>(), make<InPort>()}};
}

template <>
inline Multiplier Context::make<Multiplier>() {
    return Multiplier{m_next_id(), make<OutPort>(), {make<InPort>(), make<InPort>()}};
}

template <>
inline Register Context::make<Register>() {
    return Register{m_next_id(), make<OutPort>(), {make<InPort>()}};
}

inline std::ostream& operator<<(std::ostream& os, const InPort& port) {
    return os << "INPORT<" << util::asInt(port.id) << ">";
}

inline std::ostream& operator<<(std::ostream& os, const OutPort& port) {
    return os << "OUTPORT<" << util::asInt(port.id) << ">";
}

inline std::ostream& operator<<(std::ostream& os, const Input& input) {
    os << "INPUT<" << util::asInt(input.id) << ">"
       << "<Out:" << input.out << ">";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Output& output) {
    os << "OUTPUT<" << util::asInt(output.id) << "> ";
    for (auto& port : output.in)
       os << "<In:" << port << ">";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Adder& adder) {
    os << "ADDER<" << util::asInt(adder.id) << "> "
       << "<Out:" << adder.out << ">";
    for (auto& port : adder.in)
       os << "<In:" << port << ">";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Multiplier& multiplier) {
    os << "MULTIPLIER<" << util::asInt(multiplier.id) << "> "
       << "<Out:" << multiplier.out << ">";
    for (auto& port : multiplier.in)
       os << "<In:" << port << ">";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Register& reg) {
    os << "REG<" << util::asInt(reg.id) << "> "
       << "<Out:" << reg.out << ">";
    for (auto& port : reg.in)
       os << "<In:" << port << ">";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Device& dev) {
    return std::visit([&](auto& value) -> std::ostream& {
        return os << value;
    }, dev);
}

} // namespace dev

} // namespace exprc 

#endif // EXPRC_DEV_H
