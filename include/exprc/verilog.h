#ifndef EXPRC_VERILOG_H
#define EXPRC_VERILOG_H

#include <ostream>

#include <exprc/alloc.h>

namespace exprc {

namespace verilog {

void dump(std::ostream&, const DataPath&);

} // namespace verilog

} // namespace exprc

#endif // EXPRC_VERILOG_H
