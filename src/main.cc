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

bool Application::ScriptFileContext::open_file()
{
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

bool Application::ScriptFileContext::lex()
{
  Lexer lexer{this->source_code};

  this->token_list = lexer.lex();

  return Error::was_emitted();
}

bool Application::ScriptFileContext::parse()
{
  Parser parser{this->token_list};

  this->ast = parser.parse();

  return Error::was_emitted();
}

bool Application::ScriptFileContext::check()
{
  Sema sema{this->ast};

  sema.check();

  return Error::was_emitted();
}

Application::ScriptFileContext::ScriptFileContext(
    std::string const& path)
    : file_path(path),
      ast(nullptr)
{
}

Application::ScriptFileContext::~ScriptFileContext()
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

std::string const& Application::get_source_code()
{
  return this->source_code;
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
      this->file_path = std::move(arg);
    }
  }

  if (this->file_path.empty()) {
    std::cout << "metro: no input files.\n";
    return -1;
  }

  Application::initialize();

  this->source_code = open_file(this->file_path);

  //
  // 字句解析
  Lexer lexer{this->source_code};

  alert;
  auto xx = lexer.lex();

  // 構文解析
  Parser parser{xx};

  alert;
  auto ast = parser.parse();

  alertmsg(ast->to_string());

  chkerr;

  // 意味解析
  Sema sema{ast};

  alert;
  auto type = sema.check(ast);

  alert;
  alertmsg("check(ast) = " << type.to_string());

  chkerr;

  alert;
  auto res = Evaluator().evaluate(ast);

  debug(printf("AAA %p\n", res));

  delete res;

  alert;
  debug(_show_all_obj());

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