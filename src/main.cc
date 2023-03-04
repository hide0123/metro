#include <iostream>
#include <fstream>
#include <codecvt>
#include <locale>

#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Object.h"

#include "Lexer.h"
#include "Parser.h"
#include "Sema.h"
#include "Evaluator.h"

#include "Application.h"
#include "Error.h"

/**
 * @brief テキストファイルを開く
 *
 * @param path
 * @return std::string
 */
std::string open_file(std::string const& path)
{
  std::ifstream ifs{path};

  if (ifs.fail()) {
    std::cout << "fatal: cannot open '" << path << "'"
              << std::endl;

    std::exit(1);
  }

  std::string source;

  for (std::string line; std::getline(ifs, line);)
    source += line + '\n';

  return source;
}

void _show_all_obj();

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

bool ScriptFileContext::open_file()
{
  if (this->is_open)
    return false;

  std::ifstream ifs{this->file_path};

  if (ifs.fail()) {
    std::cout << "fatal: cannot open '" << this->file_path << "'"
              << std::endl;

    return false;
  }

  for (std::string line; std::getline(ifs, line);) {
    this->source_code += line + '\n';
  }

  return true;
}

bool ScriptFileContext::lex()
{
  Lexer lexer{this->source_code};

  this->token_list = lexer.lex();

  for (auto&& token : this->token_list) {
    token.src_loc.context = this;
  }

  return Error::was_emitted();
}

bool ScriptFileContext::parse()
{
  Parser parser{this->token_list};

  this->ast = parser.parse();

  return Error::was_emitted();
}

bool ScriptFileContext::check()
{
  Sema sema{this->ast};

  sema.check(this->ast);

  return Error::was_emitted();
}

Object* ScriptFileContext::evaluate()
{
  Evaluator eval;

  return eval.evaluate(this->ast);
}

std::string const& ScriptFileContext::get_path() const
{
  return this->file_path;
}

std::string const& ScriptFileContext::get_source_code() const
{
  return this->source_code;
}

ScriptFileContext::ScriptFileContext(std::string const& path)
    : is_open(false),
      file_path(path),
      ast(nullptr)
{
}

ScriptFileContext::~ScriptFileContext()
{
}

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

Object* Application::execute_full(ScriptFileContext& context)
{
  if (!context.open_file())
    return nullptr;

  if (!context.lex())
    return nullptr;

  if (!context.parse())
    return nullptr;

  if (!context.check())
    return nullptr;

  return context.evaluate();
}

int Application::main(int argc, char** argv)
{
#define chkerr              \
  if (Error::was_emitted()) \
  return -1

  (void)argc;
  (void)argv;

  // todo: parse arguments

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-help") {
      std::cout << "usage: metro <input file>\n";
    }
    else {
    }
  }

  if (this->scripts.empty()) {
    std::cout << "metro: no input files.\n";
    return -1;
  }

  Application::initialize();

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