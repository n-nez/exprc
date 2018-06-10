#include <exprc/dfg.h>

#include <functional>
#include <iostream>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <exprc/ir.h>

namespace exprc {

Dfg::Dfg(const std::list<Instruction>& seq) {
    for (auto& instr : seq) {
        for (auto& op : instr.src) {
            auto it = m_defined_by.find(op);
            if (it == m_defined_by.end())
                throw std::invalid_argument(fmt::format("malformed sequence {} is undefined in {}", op, instr));
            m_used_by.emplace(op, instr);
        }
        if (!instr.dst)
            continue;
        auto [prev, not_defined] = m_defined_by.emplace(*instr.dst, instr);
        if (!not_defined)
            throw std::invalid_argument(fmt::format("malformed sequence {} redefines {} in {}", prev->second, *instr.dst, instr));
    }
}

Dfg Dfg::fromSequence(const std::list<Instruction>& seq) {
    return Dfg(seq);
}

} // namespace exprc
