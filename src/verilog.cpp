#include <exprc/verilog.h>

#include <ostream>
#include <unordered_set>
#include <utility>
#include <set>

#include <fmt/ostream.h>
#include <fmt/format.h>

#include <exprc/alloc.h>
#include <exprc/util.h>

namespace exprc {

namespace verilog {

namespace {

// XXX: last control step contains only output assignments and is not really executed

auto lastState(const std::map<std::tuple<uint32_t, dev::InPort::Id>, dev::OutPort::Id>& drivers) {
    return std::get<0>(drivers.rbegin()->first) - 1;
}

auto outState(const std::map<std::tuple<uint32_t, dev::InPort::Id>, dev::OutPort::Id>& drivers) {
    return std::get<0>(drivers.rbegin()->first);
}

auto msb(uint32_t val) {
    unsigned msb = 0;
    for (unsigned i = 0; i < 32; ++i)
        if (val & (1u <<  i))
            msb = std::max(msb, i);
    return msb;
}

class Dumper {
public:
    Dumper(std::ostream& os, const DataPath& data_path)
        : m_os(os)
        , m_inputs(data_path.inputs)
        , m_outputs(data_path.outputs)
        , m_adders(data_path.adders)
        , m_multipliers(data_path.multipliers)
        , m_registers(data_path.registers) {
        fillPortInfo(data_path);
        fillControlInfo(data_path);
    }

    void dump() {
        print("module exprc(\n");
        print("  input wire clk,\n");
        print("  input wire rst,\n");
        print("  input wire ena,\n");
        for (auto& input : m_inputs)
            print("  input wire [7:0] {},\n", name(input));
        for (auto& output : m_outputs)
            print("  output wire [7:0] {},\n", name(output));
        print("  output reg done,\n");
        print("  output reg ready\n");
        print(");\n\n");
        print("  localparam [0:{}]\n", m_state_msb);
        for (uint32_t state = 1; state <= m_last_state; ++state)
            print("    S{} = {}'d{}{}\n", state, m_state_msb + 1, state - 1, state != m_last_state ? "," : ";");
        for (auto& p : m_registers)
            print("  reg [7:0] {};\n", name(p.second));
        print("\n");
        for (auto& adder : m_adders) {
            for (auto& in : adder.in)
                print("  reg [7:0] {};\n", name(in));
            print("  wire [7:0] {} = {} + {};\n", name(adder.out), name(adder.in[0]), name(adder.in[1]));
            print("\n");
        }
        for (auto& multiplier : m_multipliers) {
            for (auto& in : multiplier.in)
                print("  reg [7:0] {};\n", name(in));
            print("  wire [7:0] {} = {} * {};\n", name(multiplier.out), name(multiplier.in[0]), name(multiplier.in[1]));
            print("\n");
        }
        for (auto& p : m_drivers_by_state.equal_range(m_out_state)) {
            auto [in, driver] = p.second;
            print("  assign {} = {};\n", name(in), (name(driver)));
        }
        print("\n");
        print("  reg [0:{}] state;\n", m_state_msb);
        print("  always @(posedge clk)\n");
        print("    begin\n");
        print("      if (rst)\n");
        print("        begin\n");
        print("          state <= S1;\n");
        print("          done <= 1'b0;\n");
        print("          ready <= 1'b1;\n");
        print("        end\n");
        print("    else\n");
        print("      begin\n");
        print("        case (state)\n");
        for (uint32_t state = 1; state <= m_last_state; ++state) {
            print("          S{}:\n", state);
            print("            begin\n");
            if (state == 1) {
                print("              if (ena)\n");
                print("                begin\n");
                print("                  state <= S{};\n", state == m_last_state ? 1 : state + 1);
                print("                  done <= 1'b0;\n");
                print("                  ready <= 1'b0;\n");
                print("                end\n");
            }
            else
                print("              state <= S{};\n", state == m_last_state ? 1 : state + 1);
            for (auto& p : m_drivers_by_state.equal_range(state)) {
                auto [in, driver] = p.second;
                if (isRegPort(in))
                    print("              {} <= {};\n", name(in), name(driver));
            }
            if (state == m_last_state) {
                print("              done <= 1'b1;\n");
                print("              ready <= 1'b1;\n");
            }
            print("            end\n");
        }
        print("        endcase\n");
        print("      end\n");
        print("    end\n\n");
        print("  always @(*)\n");
        print("    begin\n");
        print("      case (state)\n");
        for (uint32_t state = 1; state <= m_last_state; ++state) {
            print("        S{}:\n", state);
            print("          begin\n");
            std::unordered_set<dev::InPort::Id> assigned;
            for (auto& p : m_drivers_by_state.equal_range(state)) {
                auto [in, driver] = p.second;
                if (!isRegPort(in))
                    print("            {} = {};\n", name(in), name(driver));
                assigned.emplace(in);
            }
            for (auto in : m_in_ports)
                if (assigned.find(in) == assigned.end() && !isRegPort(in) && !isOutput(in))
                    print("            {} = 8'dX;\n", name(in));
            print("          end\n");
        }
        print("      endcase\n");
        print("    end\n\n");
        print("endmodule\n");
    }

private:
    template <typename... Args>
    void print(Args&&... args) {
        fmt::print(m_os, std::forward<Args>(args)...);
    }

    void fillPortInfo(const DataPath& data_path) {
        auto add = [&](auto& device) {
            if constexpr (!std::is_same_v<std::decay_t<decltype(device)>, dev::Output>)
                m_device_by_port.emplace(device.out, device);
            for (auto& in : device.in) {
                m_device_by_port.emplace(in, device);
                m_in_ports.emplace(in);
            }
        };
        for (auto& input : data_path.inputs)
            add(input);
        for (auto& output : data_path.outputs) {
            add(output);
            m_output_ports.emplace(output.in[0]);
        }
        for (auto& p : data_path.registers) {
            add(p.second);
            m_reg_ports.emplace(p.second.in[0]);
        }
        for (auto& adder : data_path.adders)
            add(adder);
        for (auto& multiplier : data_path.multipliers)
            add(multiplier);
    }

    void fillControlInfo(const DataPath& data_path) {
        m_last_state = lastState(data_path.drivers);
        m_out_state = outState(data_path.drivers);
        m_state_msb = msb(m_last_state);
        for (auto& [assign, driver] : data_path.drivers) {
            auto& [step, port] = assign;
            m_drivers_by_state.emplace(step, std::make_tuple(port, driver));
        }
    }

    bool isRegPort(const dev::InPort::Id& port) {
        return m_reg_ports.find(port) != m_reg_ports.end();
    }

    bool isOutput(const dev::InPort::Id& port) {
        return m_output_ports.find(port) != m_output_ports.end();
    }

    std::string name(const dev::Input& input) {
        return input.name;
    }

    std::string name(const dev::Output& input) {
        return input.name;
    }

    std::string name(const dev::InPort::Id& port) {
        return std::visit([&](auto& device) {
            using Device = std::decay_t<decltype(device.get())>;
            if constexpr (std::is_same_v<Device, dev::Register> || std::is_same_v<Device, dev::Output>)
                return name(device);
            else
                return fmt::format("{}_in{}", name(device), util::asInt(port));
        }, m_device_by_port.at(port));
    }

    std::string name(const dev::OutPort::Id& port) {
        return std::visit([&](auto& device) {
            return name(device);
        }, m_device_by_port.at(port));
    }

    std::string name(const dev::Register& reg) {
        return fmt::format("reg{}", util::asInt(reg.id));
    }

    std::string name(const dev::Adder& adder) {
        return fmt::format("add{}", util::asInt(adder.id));
    }

    std::string name(const dev::Multiplier& multiplier) {
        return fmt::format("mul{}", util::asInt(multiplier.id));
    }

    std::ostream& m_os;
    const std::list<dev::Input>& m_inputs;
    const std::list<dev::Output>& m_outputs;
    const std::list<dev::Adder>& m_adders;
    const std::list<dev::Multiplier>& m_multipliers;
    const std::unordered_map<dev::DeviceId, dev::Register>& m_registers;
    std::unordered_map<
        std::variant<
            dev::InPort::Id,
            dev::OutPort::Id
        >,
        std::variant<
            std::reference_wrapper<const dev::Input>,
            std::reference_wrapper<const dev::Output>,
            std::reference_wrapper<const dev::Register>,
            std::reference_wrapper<const dev::Adder>,
            std::reference_wrapper<const dev::Multiplier>
        >
    > m_device_by_port;
    std::unordered_set<dev::InPort::Id> m_reg_ports;
    std::unordered_set<dev::InPort::Id> m_output_ports;
    std::set<dev::InPort::Id> m_in_ports;
    std::multimap<uint32_t, std::tuple<dev::InPort::Id, dev::OutPort::Id>> m_drivers_by_state;
    uint32_t m_last_state;
    uint32_t m_out_state;
    unsigned m_state_msb;
};

} // namespace

void dump(std::ostream& os, const DataPath& data_path) {
    Dumper(os, data_path).dump();
}

} // namespace verilog

} // namespace exprc
