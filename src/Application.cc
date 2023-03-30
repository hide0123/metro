#include <iostream>
#include <fstream>
#include <codecvt>
#include <locale>

#include "Utils.h"
#include "Debug.h"

#include "AST/AST.h"
#include "Object.h"

#include "ScriptFileContext.h"
#include "Application.h"
#include "Error.h"

static char const* g_help_string = R"(
usage:
  metro [options] [input files]
)";

static Application* _g_inst;

Application::Application()
  : _cur_ctx(nullptr)
{
  _g_inst = this;
}

Application::~Application()
{
  _g_inst = nullptr;
}

int Application::main(int argc, char** argv)
{
  Application::initialize();

  // no arguments
  if (argc == 1) {
    std::cout << g_help_string;
    return 0;
  }

  // parse arguments
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    // option
    if (arg.starts_with("--")) {
      arg = arg.substr(2);

      if (arg.empty()) {
        Error::fatal_error("missing option name");
      }

      if (arg == "help") {
        std::cout << g_help_string;
        return 0;
      }
      else {
        Error::fatal_error("unknown option name: '", arg, "'");
      }
    }

    // script file
    else if (arg.ends_with(".metro")) {
      this->add_context(arg);
    }

    // other
    else {
      Error::fatal_error("unknown argument: ", arg);
    }
  }

  // no input file
  if (this->_contexts.empty()) {
    std::cout << "metro: no input files.\n";
    return -1;
  }

  // execute
  for (auto&& script : this->_contexts) {
    this->_cur_ctx = &script;

    script.execute_full();
  }

  return 0;
}

ScriptFileContext const* Application::get_context(std::string const& path) const
{
  for (auto&& ctx : this->_contexts) {
    if (ctx.get_path() == path)
      return &ctx;

    if (auto p = ctx.is_imported(path); p)
      return p;
  }

  return nullptr;
}

ScriptFileContext const* Application::get_current_context() const
{
  return this->_cur_ctx;
}

// 初期化
void Application::initialize()
{
}

Application* Application::get_instance()
{
  return _g_inst;
}

ScriptFileContext& Application::add_context(std::string const& path)
{
  if (!std::ifstream(path).good()) {
    Error::fatal_error("cannot open file '", path, "'");
  }

  return this->_contexts.emplace_back(path);
}
