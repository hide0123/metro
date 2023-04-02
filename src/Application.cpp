#include <iostream>
#include <fstream>
#include <ctime>

#include "Utils.h"
#include "Debug.h"

#include "AST/AST.h"
#include "Object/Object.h"

#include "ScriptFileContext.h"
#include "Application.h"
#include "Error.h"

static char const* g_help_string = R"(
usage:
    metro [options] [input files]

options:
    --help, -h      show this messages
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

    if (arg == "--help" || arg == "-h") {
      std::cout << g_help_string;

#if METRO_DEBUG
      std::cout << "\n"
                << "debug options to enable macro (use -d):\n"
                   "    a      alert\n"
                   "    c      alert in constructor\n"
                   "    d      alert in destructor\n"
                   "    D      debug scope\n"
                   "    P      show syntax tree after parsed\n"
                   "    C      show syntax tree after checked semantics\n"
                   "    x      show ast infomations in option P/C\n"
                   "    y      show token infomations in option P/C\n"
                   "    g      enable text coloring of option P/C\n"
                   "    X      all";
#endif

      return 0;
    }

    // option
    else if (arg.starts_with("--")) {
      arg = arg.substr(2);

      if (arg.empty()) {
        Error::fatal_error("missing option name");
      }

      Error::fatal_error("unknown option name: '", arg, "'");
    }

#if METRO_DEBUG
    else if (arg.starts_with("-debug-")) {
      static std::pair<char, bool*> const charflagmap[] = {
        {'a', &this->_debug.flags.Alert},
        {'c', &this->_debug.flags.AlertConstructor},
        {'d', &this->_debug.flags.AlertDestructor},
        {'d', &this->_debug.flags.DebugScope},
        {'P', &this->_debug.flags.ShowASTAfterParsed},
        {'C', &this->_debug.flags.ShowASTAfterChecked},
        {'x', &this->_debug.flags.AddASTInfo},
        {'y', &this->_debug.flags.AddTokenInfo},
        {'g', &this->_debug.flags.ColoringTreeView},
      };

      arg = arg.substr(7);

      for (auto&& ch : arg) {
        if (ch == 'X') {
          for (auto&& [c, b] : charflagmap) {
            *b = true;
          }

          std::cout << COL_BOLD COL_RED
            "\t[warn: enabled all debug flags]" COL_DEFAULT "\n";

          break;
        }

        for (auto&& [c, p] : charflagmap) {
          if (c == ch) {
            *p = true;
            goto _endloop;
          }
        }

        Error::fatal_error("unknown debug option name: '", ch, "'");
      _endloop:;
      }
    }
#endif

    else if (arg == ("-c")) {
      if (i == argc - 1) {
        Error::fatal_error("missing expression");
      }

      this->_contexts.emplace_back(ScriptFileContext::from_cmdline(argv[++i]));
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
  srand((unsigned)time(nullptr));
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
