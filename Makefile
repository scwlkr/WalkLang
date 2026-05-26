CXX ?= c++
CC ?= cc
AR ?= ar
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Werror -pedantic
CFLAGS ?= -Wall -Wextra
LDFLAGS ?=
WALK_VERSION ?= dev
WALK_RELEASE_MODE ?= debug
VERSION ?= $(WALK_VERSION)
OUT ?= dist

BUILD_DIR := build
CPP_BUILD_DIR := $(BUILD_DIR)/cpp/$(subst /,_,$(WALK_VERSION))
WALK := $(BUILD_DIR)/walk
CPP_TEST_TMP := $(BUILD_DIR)/cpp-tests

CPPFLAGS += -Icompiler -DWALK_VERSION=\"$(WALK_VERSION)\"

WALK_CPP_SOURCES := \
	compiler/main.cpp \
	compiler/cli/command.cpp \
	compiler/ast/ast.cpp \
	compiler/codegen/c/c_emitter.cpp \
	compiler/codegen/c/name_mangle.cpp \
	compiler/debug_map/debug_map.cpp \
	compiler/docs/api_docs.cpp \
	compiler/docs/sitegen.cpp \
	compiler/format/format.cpp \
	compiler/ir/ir.cpp \
	compiler/ir/lower.cpp \
	compiler/lex/lexer.cpp \
	compiler/lsp/server.cpp \
	compiler/package/package.cpp \
	compiler/parse/parser.cpp \
	compiler/project/project.cpp \
	compiler/repl/repl.cpp \
	compiler/sema/builtins.cpp \
	compiler/sema/checker.cpp \
	compiler/sema/modules.cpp \
	compiler/sema/scope.cpp \
	compiler/sema/types.cpp \
	compiler/support/diagnostic.cpp \
	compiler/support/source_file.cpp \
	compiler/support/toml_like.cpp

WALK_CPP_OBJECTS := $(WALK_CPP_SOURCES:%.cpp=$(CPP_BUILD_DIR)/%.o)

TEST_CPP_SOURCES := \
	tests/cpp/test_main.cpp \
	tests/cpp/checker_tests.cpp \
	tests/cpp/emitter_tests.cpp \
	tests/cpp/lexer_tests.cpp \
	tests/cpp/module_tests.cpp \
	tests/cpp/package_tests.cpp \
	tests/cpp/parser_tests.cpp \
	tests/cpp/project_tests.cpp \
	tests/cpp/format_tests.cpp \
	tests/cpp/docs_tests.cpp \
	tests/cpp/lsp_tests.cpp \
	tests/cpp/repl_tests.cpp \
	compiler/cli/command.cpp \
	compiler/ast/ast.cpp \
	compiler/codegen/c/c_emitter.cpp \
	compiler/codegen/c/name_mangle.cpp \
	compiler/debug_map/debug_map.cpp \
	compiler/docs/api_docs.cpp \
	compiler/docs/sitegen.cpp \
	compiler/format/format.cpp \
	compiler/ir/ir.cpp \
	compiler/ir/lower.cpp \
	compiler/lex/lexer.cpp \
	compiler/lsp/server.cpp \
	compiler/package/package.cpp \
	compiler/parse/parser.cpp \
	compiler/project/project.cpp \
	compiler/repl/repl.cpp \
	compiler/sema/builtins.cpp \
	compiler/sema/checker.cpp \
	compiler/sema/modules.cpp \
	compiler/sema/scope.cpp \
	compiler/sema/types.cpp \
	compiler/support/diagnostic.cpp \
	compiler/support/source_file.cpp \
	compiler/support/toml_like.cpp

TEST_CPP_OBJECTS := $(TEST_CPP_SOURCES:%.cpp=$(CPP_BUILD_DIR)/%.o)
TEST_CPP_BIN := $(CPP_BUILD_DIR)/walk-tests

.PHONY: walk test conformance docs check-docs release install-local clean

walk: $(WALK)

$(WALK): $(WALK_CPP_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

$(CPP_BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

test: $(TEST_CPP_BIN)
	$(TEST_CPP_BIN)

$(TEST_CPP_BIN): $(TEST_CPP_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

conformance: walk
	WALK_REF=$$PWD/$(WALK) tests/conformance/run.sh --verify

docs:
	scripts/build-docs-site.sh

check-docs:
	scripts/check-docs-site.sh

release:
	@if [ "$(VERSION)" = "dev" ]; then echo "VERSION is required" >&2; exit 2; fi
	scripts/release.sh "$(VERSION)" "$(OUT)"

install-local:
	scripts/install-local.sh "$(VERSION)"

clean:
	rm -rf $(BUILD_DIR)/cpp $(CPP_TEST_TMP) $(WALK)
