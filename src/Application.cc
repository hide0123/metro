#include <iostream>
#include <fstream>
#include <codecvt>
#include <locale>

#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Object.h"

#include "ScriptFileContext.h"
#include "Application.h"
#include "Error.h"

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

  // parse arguments
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-help") {
      std::cout << "usage: metro <input file>\n";
    }
    else if (arg.ends_with(".metro")) {
      if(!std::ifstream(arg).good()){
        std::cerr << "fatal: cannot open file '" << arg << "'"
                  << std::endl;
        return -1;
      }

      this->_contexts.emplace_back(arg);
    }
    else {
      std::cerr << "fatal: unknown argument: " << arg
                << std::endl;

      return -1;
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

ScriptFileContext const* Application::get_context(
    std::string const& path) const
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
  Object::initialize();
}

Application* Application::get_instance()
{
  return _g_inst;
}