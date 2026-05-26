#pragma once

#include "ir/ir.h"
#include "sema/modules.h"

#include <map>
#include <memory>
#include <string>

namespace walk::ir {

struct LoweredProgram {
    Program program;
    std::map<std::string, std::unique_ptr<Program>> modules;
};

[[nodiscard]] LoweredProgram lower_program(ast::Program& program, const std::map<std::string, std::unique_ptr<sema::Module>>& modules);

}  // namespace walk::ir
