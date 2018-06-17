#include <exprc/translate.h>

#include <list>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>
#include <unordered_map>

#include <fmt/format.h>

#include <exprc/ir.h>
#include <exprc/parse.h>

namespace exprc {

namespace {

class Translate {
public:
    auto translate(const std::list<ast::Assign>& program) {
        for (auto& assign : program)
            translateAssign(assign);
        return std::make_tuple(std::move(m_sequence), std::move(m_name_by_oper));
    }

private:
    void translateAssign(const ast::Assign& assign) {
        std::visit([&](auto& assign) {
            translateAssign(assign);
        }, assign);
    }

    void translateAssign(const ast::AssignVar& assign) {
        auto res = translateExpr(*assign.expr);
        auto redefined = !m_oper_by_name.emplace(assign.name, res).second;
        if (redefined)
            throw std::invalid_argument(fmt::format("variable {} defined more than once", assign.name));
        m_name_by_oper.emplace(res, assign.name);
    }

    void translateAssign(const ast::AssignOut& assign) {
        auto res = translateExpr(*assign.expr);
        auto redefined = !m_oper_by_name.emplace(assign.name, res).second;
        if (redefined)
            throw std::invalid_argument(fmt::format("output variable {} defined more than once", assign.name));
        m_name_by_oper.emplace(res, assign.name);
        addInstr(Opcode::OUTPUT, std::optional<Operand>(), std::vector({res}));
    }

    Operand translateExpr(const ast::Expr& expr) {
        return std::visit([&](auto& expr) {
            return translateExpr(expr);
        }, expr);
    }

    Operand translateExpr(const ast::Add& add) {
        auto opA = translateExpr(*add.a);
        auto opB = translateExpr(*add.b);
        auto res = m_context.make<Operand>();
        addInstr(Opcode::ADD, res, std::vector({opA, opB}));
        return res;
    }

    Operand translateExpr(const ast::Mul& mul) {
        auto opA = translateExpr(*mul.a);
        auto opB = translateExpr(*mul.b);
        auto res = m_context.make<Operand>();
        addInstr(Opcode::MUL, res, std::vector({opA, opB}));
        return res;
    }

    Operand translateExpr(const ast::Var& var) {
        auto it = m_oper_by_name.find(var.name);
        if (it != m_oper_by_name.end())
            return it->second;
        auto op = m_context.make<Operand>();
        addInstr(Opcode::INPUT, op, std::vector<Operand>());
        m_oper_by_name.emplace(var.name, op);
        m_name_by_oper.emplace(op, var.name);
        return op;
    }

    template <typename... Args>
    void addInstr(Args&&... args) {
        m_sequence.emplace_back(m_context.make<Instruction>(std::forward<Args>(args)...));
    }

    Context m_context;
    std::list<Instruction> m_sequence;
    std::unordered_map<std::string, const Operand> m_oper_by_name;
    std::unordered_map<Operand::Id, const std::string> m_name_by_oper;
};

} // namespace

std::tuple<std::list<Instruction>, std::unordered_map<Operand::Id, const std::string>> translate(const std::list<ast::Assign>& program) {
    return Translate().translate(program);
}

} // namespace exprc
