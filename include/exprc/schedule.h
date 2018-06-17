#ifndef EXPRC_SCHEDULE_H
#define EXPRC_SCHEDULE_H

#include <map>

#include <exprc/dfg.h>
#include <exprc/ir.h>

namespace exprc {

std::multimap<uint32_t, std::reference_wrapper<const Instruction>> schedule(const std::list<Instruction>&, const Dfg&);

} // namespace exprc

#endif // EXPRC_SCHEDULE_H
