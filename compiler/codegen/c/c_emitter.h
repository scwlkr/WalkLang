#pragma once

#include "ir/lower.h"
#include "support/result.h"

#include <string>

namespace walk::codegen::c {

struct EmitOptions {
    bool tests_only = false;
};

[[nodiscard]] Result<std::string> emit_c(const ir::LoweredProgram& lowered, EmitOptions options = {});

}  // namespace walk::codegen::c
