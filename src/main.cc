#include <iostream>
#include <fstream>
#include <codecvt>
#include <locale>

#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Object.h"

#include "Application.h"
#include "Error.h"

namespace Utils::String {

static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>
    conv;

std::wstring to_wstr(std::string const& str)
{
  return conv.from_bytes(str);
}

std::string to_str(std::wstring const& str)
{
  return conv.to_bytes(str);
}

}  // namespace Utils::String

static Application* app_inst;

Application::Application()
{
  ::app_inst = this;
}

Application::~Application()
{
  ::app_inst = nullptr;
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

void Application::execute_full(ScriptFileContext& context)
{
  if (!context.open_file()) {
    std::cout << "metro: cannot open file '" << context.file_path
              << "'" << std::endl;

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

int main(int argc, char** argv)
{
  try {
    return Application().main(argc, argv);
  }
  catch (std::exception e) {
    alertmsg(e.what());
  }
  catch (...) {
    alert;
  }

  return -1;
}