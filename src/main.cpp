#include <iostream>

#include <exprc/ir.h>

int main() {
    exprc::Context context;
    std::list<exprc::Instruction> sequence;

    auto add = [&](auto&&... args) {
        sequence.emplace_back(context.make<exprc::Instruction>(std::forward<decltype(args)>(args)...));
    };

    // C = A + B
    // F = A + D
    // G = C * F
    auto opA = context.make<exprc::Operand>();
    add(exprc::Opcode::INPUT, opA, std::vector<exprc::Operand>());
    auto opB = context.make<exprc::Operand>();
    add(exprc::Opcode::INPUT, opB, std::vector<exprc::Operand>());
    auto opC = context.make<exprc::Operand>();
    add(exprc::Opcode::ADD, opC, std::vector({opA, opB}));
    auto opD = context.make<exprc::Operand>();
    add(exprc::Opcode::INPUT, opD, std::vector<exprc::Operand>());
    auto opF = context.make<exprc::Operand>();
    add(exprc::Opcode::ADD, opF, std::vector({opA, opD}));
    auto opG = context.make<exprc::Operand>();
    add(exprc::Opcode::MUL, opG, std::vector({opC, opF}));
    add(exprc::Opcode::OUTPUT, std::optional<exprc::Operand>(), std::vector({opG}));

    for (auto& instr : sequence)
        std::cout << instr << std::endl;
    std::cout << std::endl;

    return 0;
}
