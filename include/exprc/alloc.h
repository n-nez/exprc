#ifndef EXPRC_ALLOC_H
#define EXPRC_ALLOC_H

#include <functional>
#include <map>
#include <unordered_map>
#include <list>

#include <exprc/dev.h>
#include <exprc/ir.h>

namespace exprc {

struct DataPath {
    std::list<dev::Input> inputs;
    std::list<dev::Output> outputs;
    std::list<dev::Adder> adders;
    std::list<dev::Multiplier> multipliers;
    std::unordered_map<dev::DeviceId, dev::Register> registers;
    std::map<std::tuple<uint32_t, dev::InPort::Id>, dev::OutPort::Id> drivers;
};

DataPath allocate(const std::multimap<uint32_t, std::reference_wrapper<const Instruction>>&);

} // namespace exprc

#endif // EXPRC_ALLOC_H
