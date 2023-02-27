#include <iostream>
#include <fstream>
#include <codecvt>
#include <locale>

#include "common.h"

#include "AST.h"
#include "Object.h"

#include "Lexer.h"
#include "Parser.h"
#include "Checker.h"
#include "Compiler.h"
#include "Evaluator.h"

#include "Application.h"

namespace Utils::String {

static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>
  conv;

std::wstring to_wstr(std::string const& str) {
  return conv.from_bytes(str);
}

std::string to_str(std::wstring const& str) {
  return conv.to_bytes(str);
}

} // namespace Utils::String

static Application* app_inst;

Application::Application() {
  ::app_inst = this;
}

Application::~Application() {
  ::app_inst = nullptr;
}

std::string const& Application::get_source_code() {
  return this->source_code;
}


// 初期化
void Application::initialize() {


  Object::initialize();


}

Application& Application::get_current_instance() {
  return *::app_inst;
}

/**
 * @brief テキストファイルを開く
 * 
 * @param path 
 * @return std::string 
 */
std::string open_file(std::string const& path) {
  std::ifstream ifs{ path };

  std::string source;
  
  for( std::string line; std::getline(ifs, line); )
    source += line + '\n';

  return source;
}

int Application::main(int argc, char** argv) {
  

  Application::initialize();
  

  this->source_code = open_file("test.txt");

  //
  // 字句解析
  Lexer lexer{ this->source_code };

  auto const& token_list = lexer.lex();

  // 構文解析
  Parser parser{ token_list };

  auto ast = parser.parse();

  alertmsg(ast->to_string());

  Token toktok{TOK_End};
  toktok.str="main";

  auto azz=new AST::CallFunc(toktok);
  ast->append(azz);
  
  // 意味解析
  Checker checker{ ast };

  auto type = checker.check(ast);

  alertmsg("check(ast) = " << type.to_string());

  Compiler com;

  alert;
  com.compile(ast);


  // Evaluator eval;

  // eval.evaluate(ast);



  return 0;
}

int main(int argc, char** argv) {
  return Application().main(argc, argv);
}