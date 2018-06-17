#include <iostream>
#include <list>
#include <map>
#include <unordered_map>
#include <sstream>

#include <exprc/alloc.h>
#include <exprc/dev.h>
#include <exprc/dfg.h>
#include <exprc/ir.h>
#include <exprc/verilog.h>
#include <exprc/parse.h>
#include <exprc/schedule.h>
#include <exprc/translate.h>

int main() {
    std::stringstream ss1("C = A + B; F = A + D; out G = C * F;");
    auto [sequence, name_table] = exprc::translate(exprc::ast::parse(ss1));

    for (auto& instr : sequence)
        std::cout << instr << std::endl;
    std::cout << std::endl;

    auto dfg = exprc::Dfg::fromSequence(sequence);
    for (auto& instr : sequence) {
        std::cout << instr << " depends on: " << std::endl;
        for (auto& op : instr.src)
            std::cout << "  " << dfg.definedBy(op) << std::endl;
    }
    std::cout << std::endl;

    for (auto& instr : sequence) {
        std::cout << instr << " used by: " << std::endl;
        if (!instr.dst)
            continue;
        for (auto p : dfg.usedBy(*instr.dst)) {
            std::cout << "  " << p.second.get() << std::endl;
        }
    }
    std::cout << std::endl;

    auto sched = schedule(sequence, dfg);
    for (auto& [step, instr] : sched)
        std::cout << step << ": " << instr << std::endl;
    std::cout << std::endl;

    auto data_path = exprc::allocate(sched, name_table);
    exprc::verilog::dump(std::cout, data_path);

    return 0;
}
