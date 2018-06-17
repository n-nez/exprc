#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <unordered_map>
#include <sstream>

#include <fmt/format.h>

#include <exprc/alloc.h>
#include <exprc/dev.h>
#include <exprc/dfg.h>
#include <exprc/ir.h>
#include <exprc/verilog.h>
#include <exprc/parse.h>
#include <exprc/schedule.h>
#include <exprc/translate.h>

namespace {

void usage() {
    std::cout << "exprc [-d] prog.txt" << std::endl;
    std::cout << "    -d  dump debug information" << std::endl;
}

void checkForDeadCode(const std::list<exprc::Instruction>& sequence, const exprc::Dfg& dfg, const std::unordered_map<exprc::Operand::Id, const std::string>& name_table) {
    auto empty = [](const auto& range) {
        return range.first == range.second;
    };
    for (auto& instr : sequence) {
        if (!instr.dst)
            continue;
        auto& dst = *instr.dst;
        if (empty(dfg.usedBy(dst)))
            throw std::invalid_argument(fmt::format("variable {} is not used neither in 'out' statement nor in another expression", name_table.at(dst)));
    }
}

void doAll(bool debug, const char* file) {
    std::ifstream stream(file);
    auto [sequence, name_table] = exprc::translate(exprc::ast::parse(stream));

    if (debug) {
        for (auto& instr : sequence)
            std::cout << instr << std::endl;
        std::cout << std::endl;
    }

    auto dfg = exprc::Dfg::fromSequence(sequence);
    if (debug) {
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
    }

    checkForDeadCode(sequence, dfg, name_table);

    auto sched = schedule(sequence, dfg);
    if (debug) {
        for (auto& [step, instr] : sched)
            std::cout << step << ": " << instr << std::endl;
        std::cout << std::endl;
    }

    auto data_path = exprc::allocate(sched, name_table);
    exprc::verilog::dump(std::cout, data_path);
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        usage();
        return 1;
    }

    bool debug = false;
    auto* file = argv[1];
    if (argc == 3) {
        if (argv[1] != std::string("-d")) {
            usage();
            return 1;
        }
        debug = true;
        file = argv[2];
    }

    try {
        doAll(debug, file);
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
