#include "repl/repl.h"

#include <sstream>

namespace walk::repl {

std::string source_for_expression(const std::string& expression) {
    std::ostringstream out;
    out << "imp: math\n";
    out << "imp: string\n";
    out << "imp: array\n";
    out << "imp: random\n";
    out << "imp: time\n";
    out << "imp: testing\n";
    out << "out: " << expression << "\n\n";
    return out.str();
}

}  // namespace walk::repl
