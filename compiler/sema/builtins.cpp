#include "sema/builtins.h"

#include "sema/types.h"

#include <map>
#include <string>
#include <utility>

namespace walk::sema {
namespace {

ast::StructField field(std::string name, ast::Type type) {
    return ast::StructField{{}, std::move(name), std::move(type)};
}

ast::Type bool_type() {
    return ast::basic(ast::TypeKind::Bool);
}

ast::Type int_type() {
    return ast::basic(ast::TypeKind::Int);
}

ast::Type float_type() {
    return ast::basic(ast::TypeKind::Float);
}

ast::Type string_type() {
    return ast::basic(ast::TypeKind::String);
}

ast::Type void_type() {
    return ast::basic(ast::TypeKind::Void);
}

std::vector<BuiltinStruct> make_structs() {
    return {
        {"IOReadResult", {field("ok", bool_type()), field("value", string_type()), field("error", string_type())}, true},
        {"ParseIntResult", {field("ok", bool_type()), field("value", int_type()), field("error", string_type())}, true},
        {"ParseFloatResult", {field("ok", bool_type()), field("value", float_type()), field("error", string_type())}, true},
        {"ParseBoolResult", {field("ok", bool_type()), field("value", bool_type()), field("error", string_type())}, true},
        {"FileReadResult", {field("ok", bool_type()), field("value", string_type()), field("error", string_type())}, true},
        {"FileActionResult", {field("ok", bool_type()), field("value", bool_type()), field("error", string_type())}, true},
        {"ProcessResult", {field("ok", bool_type()), field("status", int_type()), field("stdout", string_type()), field("stderr", string_type()), field("error", string_type())}, true},
        {"ProcessOutputResult", {field("ok", bool_type()), field("value", string_type()), field("status", int_type()), field("error", string_type())}, true},
        {"JsonResult", {field("ok", bool_type()), field("value", string_type()), field("error", string_type())}, true},
        {"HttpResult", {field("ok", bool_type()), field("status", int_type()), field("body", string_type()), field("error", string_type())}, true},
    };
}

BuiltinFunction fn(std::string module, std::string name, std::vector<ast::Type> params, ast::Type ret, bool effect = false) {
    return {std::move(module), std::move(name), std::move(params), std::move(ret), effect, true};
}

std::vector<BuiltinFunction> make_functions() {
    return {
        fn("io", "write", {string_type()}, void_type(), true),
        fn("io", "write_line", {string_type()}, void_type(), true),
        fn("io", "error_line", {string_type()}, void_type(), true),
        fn("io", "read_line", {}, ast::struct_type("IOReadResult")),
        fn("io", "read_all", {}, ast::struct_type("IOReadResult")),
        fn("file", "read", {string_type()}, string_type()),
        fn("file", "try_read", {string_type()}, ast::struct_type("FileReadResult")),
        fn("file", "write", {string_type(), string_type()}, void_type(), true),
        fn("file", "try_write", {string_type(), string_type()}, ast::struct_type("FileActionResult")),
        fn("file", "append", {string_type(), string_type()}, void_type(), true),
        fn("file", "try_append", {string_type(), string_type()}, ast::struct_type("FileActionResult")),
        fn("file", "exists", {string_type()}, bool_type()),
        fn("dir", "list", {string_type()}, ast::array_of(string_type())),
        fn("dir", "make", {string_type()}, void_type(), true),
        fn("dir", "delete", {string_type()}, void_type(), true),
        fn("path", "join", {string_type(), string_type()}, string_type()),
        fn("path", "base", {string_type()}, string_type()),
        fn("path", "ext", {string_type()}, string_type()),
        fn("process", "args", {}, ast::array_of(string_type())),
        fn("process", "arg_count", {}, int_type()),
        fn("process", "env", {string_type()}, nullable_string()),
        fn("process", "cwd", {}, string_type()),
        fn("process", "chdir", {string_type()}, void_type(), true),
        fn("process", "run", {string_type(), ast::array_of(string_type())}, ast::struct_type("ProcessResult")),
        fn("process", "output", {string_type(), ast::array_of(string_type())}, ast::struct_type("ProcessOutputResult")),
        fn("process", "run_shell", {string_type()}, ast::struct_type("ProcessResult")),
        fn("process", "exit", {int_type()}, void_type(), true),
        fn("parse", "int", {string_type()}, ast::struct_type("ParseIntResult")),
        fn("parse", "float", {string_type()}, ast::struct_type("ParseFloatResult")),
        fn("parse", "bool", {string_type()}, ast::struct_type("ParseBoolResult")),
        fn("json", "parse", {string_type()}, ast::struct_type("JsonResult")),
        fn("json", "stringify", {string_type()}, string_type()),
        fn("json", "read", {string_type()}, ast::struct_type("JsonResult")),
        fn("json", "write", {string_type(), string_type()}, void_type(), true),
        fn("term", "is_tty", {}, bool_type()),
        fn("term", "color", {string_type()}, void_type(), true),
        fn("term", "background", {string_type()}, void_type(), true),
        fn("term", "style", {string_type()}, void_type(), true),
        fn("term", "reset", {}, void_type(), true),
        fn("term", "clear", {}, void_type(), true),
        fn("term", "move", {int_type(), int_type()}, void_type(), true),
        fn("term", "width", {}, int_type()),
        fn("term", "height", {}, int_type()),
        fn("term", "read_key", {}, ast::struct_type("IOReadResult")),
        fn("http", "get", {string_type()}, ast::struct_type("HttpResult")),
        fn("http", "post", {string_type(), string_type()}, ast::struct_type("HttpResult")),
        fn("http", "request", {string_type(), string_type(), string_type()}, ast::struct_type("HttpResult")),
        fn("html", "escape", {string_type()}, string_type()),
        fn("html", "h1", {string_type()}, string_type()),
        fn("html", "p", {string_type()}, string_type()),
        fn("html", "button", {string_type()}, string_type()),
    };
}

const std::vector<BuiltinFunction>& functions() {
    static const std::vector<BuiltinFunction> table = make_functions();
    return table;
}

const std::map<std::string, const BuiltinFunction*>& function_index() {
    static const std::map<std::string, const BuiltinFunction*> index = [] {
        std::map<std::string, const BuiltinFunction*> out;
        for (const BuiltinFunction& builtin : functions()) {
            out.emplace(builtin.qualified_name(), &builtin);
        }
        return out;
    }();
    return index;
}

}  // namespace

std::string BuiltinFunction::qualified_name() const {
    return module + "." + name;
}

const BuiltinFunction* lookup_builtin(const std::string& module, const std::string& name) {
    return lookup_qualified_builtin(module + "." + name);
}

const BuiltinFunction* lookup_qualified_builtin(const std::string& qualified) {
    const auto found = function_index().find(qualified);
    if (found == function_index().end()) {
        return nullptr;
    }
    return found->second;
}

bool is_builtin_module(const std::string& name) {
    if (name == "math" || name == "string" || name == "array" || name == "time" || name == "random" || name == "testing" || name == "map") {
        return true;
    }
    for (const BuiltinFunction& builtin : functions()) {
        if (builtin.module == name) {
            return true;
        }
    }
    return false;
}

const std::vector<BuiltinStruct>& builtin_structs() {
    static const std::vector<BuiltinStruct> table = make_structs();
    return table;
}

}  // namespace walk::sema
