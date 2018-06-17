#ifndef EXPRC_PARSE_H
#define EXPRC_PARSE_H

#include <istream>
#include <list>
#include <string>
#include <memory>
#include <variant>

namespace exprc {

namespace ast {

struct Var;
struct Add;
struct Mul;

using Expr = std::variant<Var, Add, Mul>;

struct Var {
    std::string name;
};

struct Add {
    std::unique_ptr<Expr> a;
    std::unique_ptr<Expr> b;
};

struct Mul {
    std::unique_ptr<Expr> a;
    std::unique_ptr<Expr> b;
};

struct AssignVar {
    std::string name;
    std::unique_ptr<Expr> expr;
};

struct AssignOut {
    std::string name;
    std::unique_ptr<Expr> expr;
};

using Assign = std::variant<AssignVar, AssignOut>;

std::list<Assign> parse(std::istream&);

} // namespace ast

} // namespace exprc

#endif // EXPRC_PARSE_H
