#include <exprc/schedule.h>

#include <algorithm>
#include <map>

#include <exprc/dfg.h>
#include <exprc/ir.h>

namespace exprc {

// generates maximally parallel schedule scheduling things as early as possible
std::multimap<uint32_t, std::reference_wrapper<const Instruction>> schedule(const std::list<Instruction>& sequence, const Dfg& dfg) {
    std::multimap<uint32_t, std::reference_wrapper<const Instruction>> schedule;
    std::unordered_map<Instruction::Id, uint32_t> earliest_step;
    auto earliest = [&](const Instruction& instr) {
        uint32_t step = 0;
        for (auto& op : instr.src)
            step = std::max(step, earliest_step.at(dfg.definedBy(op)) + 1);
        earliest_step.emplace(instr, step);
        return step;
    };
    for (auto& instr : sequence)
        if (instr.opcode != Opcode::OUTPUT)
            schedule.emplace(earliest(instr), instr);
    auto last_step = schedule.rbegin()->first + 1;
    for (auto& instr : sequence)
        if (instr.opcode == Opcode::OUTPUT)
            schedule.emplace(last_step, instr);
    return schedule;
}

} // namespace exprc
