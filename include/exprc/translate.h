#ifndef EXPRC_TRANSLATE_H
#define EXPRC_TRANSLATE_H

#include <list>
#include <string>
#include <tuple>
#include <unordered_map>

#include <exprc/ir.h>
#include <exprc/parse.h>

namespace exprc {

std::tuple<std::list<Instruction>, std::unordered_map<Operand::Id, const std::string>> translate(const std::list<ast::Assign>&);

} // namespace exprc

#endif // EXPRC_TRANSLATE_H
