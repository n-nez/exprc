#ifndef EXPRC_DFG_H
#define EXPRC_DFG_H

#include <functional>
#include <unordered_map>
#include <iterator>
#include <type_traits>

#include <exprc/ir.h>

namespace exprc {

class Dfg {
public:
    Dfg(const Dfg&) = delete;
    Dfg(Dfg&&) = default;

    auto usedBy(const Operand& op) const {
        return m_used_by.equal_range(op);
    }

    const Instruction& definedBy(const Operand& op) const {
        return m_defined_by.at(op);
    }

    static Dfg fromSequence(const std::list<Instruction>&);

private:
    Dfg(const std::list<Instruction>&);

    std::unordered_map<Operand::Id, std::reference_wrapper<const Instruction>> m_defined_by;
    std::unordered_multimap<Operand::Id, std::reference_wrapper<const Instruction>> m_used_by;
};

} // namespace

#endif // EXPRC_DFG_H
