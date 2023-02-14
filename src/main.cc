#include <iostream>
#include <fstream>
#include <codecvt>
#include <locale>

#include "AST.h"
#include "Object.h"

#include "Lexer.h"
#include "Parser.h"
#include "Checker.h"
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


Application::Application() {

}

Application::~Application() {

}

// 初期化
void Application::initialize() {


  Object::initialize();


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

  // 意味解析
  Checker checker{ ast };

  auto type = checker.check(ast);


  GarbageCollector gc; // ガベージコレクタ


  // 実行
  Evaluator evaluator;

  auto obj = evaluator.evaluate(ast);



  return 0;
}

int main(int argc, char** argv) {
  return Application().main(argc, argv);
}