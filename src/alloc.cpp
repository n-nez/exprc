#include <exprc/alloc.h>

#include <algorithm>
#include <cassert>
#include <map>
#include <list>
#include <queue>
#include <unordered_map>
#include <iostream>
#include <type_traits>

#include <exprc/ir.h>
#include <exprc/dev.h>
#include <exprc/dfg.h>

namespace exprc {

namespace {

template <typename D>
class DevicePool {
public:
    DevicePool(dev::Context& context)
        : m_context(context)
        , m_unallocated(m_list.begin()) {
    }

    auto& alloc() {
        if (m_unallocated == m_list.end()) {
            auto& dev = m_list.emplace_back(m_context.make<D>());
            m_unallocated = m_list.end();
            return dev;
        }
        return *m_unallocated++;
    }

    void reset() {
        m_unallocated = m_list.begin();
    }

    auto& list() const {
        return m_list;
    }

private:
    dev::Context& m_context;
    std::list<D> m_list;
    decltype(m_list.begin()) m_unallocated;
};

template <typename D>
class IoPool {
public:
    IoPool(dev::Context& context)
        : m_context(context) {
    }

    const auto& alloc(const std::string& name) {
        return m_list.emplace_back(m_context.make<D>(name));
    }

    auto& list() const {
        return m_list;
    }

private:
    dev::Context& m_context;
    std::list<D> m_list;
};

class RegisterPool {
public:
    RegisterPool(dev::Context& context)
        : m_context(context) {
    }

    auto alloc() {
        if (m_unallocated.empty()) {
            auto reg = m_context.make<dev::Register>();
            return m_regs.emplace(reg, reg).first->second.id;
        }
        auto reg = m_unallocated.front();
        m_unallocated.pop();
        return reg;
    }

    void put(dev::DeviceId reg) {
        m_unallocated.push(reg);
    }

    auto& regs() const {
        return m_regs;
    }

    auto& reg(dev::DeviceId id) {
        return m_regs.at(id);
    }

private:
    dev::Context& m_context;
    std::queue<dev::DeviceId> m_unallocated;
    std::unordered_map<dev::DeviceId, dev::Register> m_regs;
};

class DeviceAllocator {
public:
    DeviceAllocator(dev::Context& context, const std::multimap<uint32_t, std::reference_wrapper<const Instruction>>& schedule, const std::unordered_map<Operand::Id, const std::string>& name_by_oper)
        : m_context(context)
        , m_inputs(context)
        , m_outputs(context)
        , m_adders(context)
        , m_multipliers(context)
        , m_regs(context)
        , m_schedule(schedule)
        , m_name_by_oper(name_by_oper) {
    }

    DataPath doIt() {
        allocateRegisters();
        allocateDevices();
        return DataPath{m_inputs.list(), m_outputs.list(), m_adders.list(), m_multipliers.list(), m_regs.regs(), m_driver_list};
    }

private:
    void allocateDevices();
    void allocateRegisters();
    template <typename Device>
    void mapIn(uint32_t, const Instruction&, const Device&);
    template <typename Device>
    void mapOut(uint32_t, const Instruction&, const Device&);
    template <typename Device>
    void mapIo(uint32_t, const Instruction&, const Device&);

    const std::string& inputName(const Instruction&);
    const std::string& outputName(const Instruction&);

    dev::Context& m_context;
    IoPool<dev::Input> m_inputs;
    IoPool<dev::Output> m_outputs;
    DevicePool<dev::Adder> m_adders;
    DevicePool<dev::Multiplier> m_multipliers;
    RegisterPool m_regs;
    std::unordered_map<Operand::Id, dev::DeviceId> m_reg_mapping;
    std::unordered_map<Operand::Id, dev::OutPort::Id> m_fed_by_reg;
    std::unordered_map<Operand::Id, dev::OutPort::Id> m_fed_by_input;
    const std::multimap<uint32_t, std::reference_wrapper<const Instruction>>& m_schedule;
    const std::unordered_map<Operand::Id, const std::string>& m_name_by_oper;
    std::map<std::tuple<uint32_t, dev::InPort::Id>, dev::OutPort::Id> m_driver_list;
};

template <typename Device>
void DeviceAllocator::mapIn(uint32_t step, const Instruction& instr, const Device& device) {
    assert(instr.src.size() == device.in.size());
    for (size_t i = 0; i < instr.src.size(); ++i) {
        auto& op = instr.src[i];
        auto& in = device.in[i];
        // at first step in ports feed devices directly, later everything are fed by regs
        auto& m_fed_by = (step == 1) ? m_fed_by_input : m_fed_by_reg;
        m_driver_list.emplace(std::make_tuple(step, in), m_fed_by.at(op));
    }
}

template <>
void DeviceAllocator::mapOut(uint32_t step, const Instruction& instr, const dev::Output&) {
}

template <typename Device>
void DeviceAllocator::mapOut(uint32_t step, const Instruction& instr, const Device& device) {
    if (!instr.dst)
        return;
    auto& dst = *instr.dst;
    auto it = m_reg_mapping.find(dst);
    if (it == m_reg_mapping.end()) {
        assert(instr.opcode == Opcode::INPUT && step == 0);
        m_fed_by_input.emplace(dst, device.out);
    }
    else {
        auto& reg = m_regs.reg(it->second);
        // zero step is not really exists, so assignment should be done in first one
        m_driver_list.emplace(std::make_tuple(std::max(step,  1u), reg.in[0]), device.out);
        m_fed_by_reg.emplace(dst, reg.out);
    }
}

template <typename Device>
void DeviceAllocator::mapIo(uint32_t step, const Instruction& instr, const Device& device) {
    mapIn(step, instr, device);
    mapOut(step, instr, device);
}

const std::string& DeviceAllocator::inputName(const Instruction& input) {
    return m_name_by_oper.at(input.dst.value());
}

const std::string& DeviceAllocator::outputName(const Instruction& output) {
    return m_name_by_oper.at(output.src.at(0));
}

void DeviceAllocator::allocateDevices() {
    auto last_step = m_schedule.rbegin()->first;
    for (uint32_t step = 0; step <= last_step; ++step) {
        m_adders.reset();
        m_multipliers.reset();
        for (auto p : m_schedule.equal_range(step)) {
            const Instruction& instr = p.second;
            switch (instr.opcode) {
            case Opcode::ADD:
                mapIo(step, instr, m_adders.alloc());
                break;
            case Opcode::MUL:
                mapIo(step, instr, m_multipliers.alloc());
                break;
            case Opcode::INPUT:
                mapIo(step, instr, m_inputs.alloc(inputName(instr)));
                break;
            case Opcode::OUTPUT:
                mapIo(step, instr, m_outputs.alloc(outputName(instr)));
            }
        }
    }
}

void DeviceAllocator::allocateRegisters() {
    // zero step contains only INPUT instructions
    // first step instructions are fed by ports
    auto last_step = m_schedule.rbegin()->first;
    for (auto step = last_step; step > 1; --step)
        for (auto p : m_schedule.equal_range(step)) {
            const Instruction& instr = p.second;
            if (instr.dst)
                m_regs.put(m_reg_mapping.at(*instr.dst));
            for (auto& src : instr.src)
                if (m_reg_mapping.find(src) == m_reg_mapping.end())
                    m_reg_mapping.emplace(src, m_regs.alloc());
        }
}

} // namespace

DataPath allocate(const std::multimap<uint32_t, std::reference_wrapper<const Instruction>>& schedule, const std::unordered_map<Operand::Id, const std::string>& name_by_oper) {
    exprc::dev::Context context;
    exprc::DeviceAllocator allocator(context, schedule, name_by_oper);
    return allocator.doIt();
}

} // namespace exprc
