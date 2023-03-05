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

static Application* app_inst;

Application::Application()
{
  ::app_inst = this;
}

Application::~Application()
{
  ::app_inst = nullptr;
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
      this->scripts.emplace_back(arg);
    }
  }

  // no input file
  if (this->scripts.empty()) {
    std::cout << "metro: no input files.\n";
    return -1;
  }

  // execute
  for (auto&& script : this->scripts) {
    this->execute_full(script);
  }

  return 0;
}

void Application::execute_full(ScriptFileContext& context)
{
  if (!context.open_file()) {
    std::cout << "metro: cannot open file '"
              << context.get_path() << "'" << std::endl;

    return;
  }

  if (!context.lex())
    return;

  if (!context.parse())
    return;

  if (!context.check())
    return;

  context.evaluate();
}

// 初期化
void Application::initialize()
{
  Object::initialize();
}

Application& Application::get_current_instance()
{
  return *::app_inst;
}

ScriptFileContext const* Application::get_context_with_path(
    std::string const& path)
{
  for (auto&&)
}