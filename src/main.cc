#include <iostream>
#include <fstream>
#include <codecvt>
#include <locale>
#include "metro.h"

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

int main(int argc, char** argv) {
  
  auto const& source = open_file("test.txt");

  Application app;

  app.initialize();

  Lexer lexer{ source };

  auto const& token_list = lexer.lex();

  Parser parser{ token_list };

  auto ast = parser.parse();

  Checker checker{ ast };

  auto type = checker.check(ast);


  GarbageCollector gc;

  Evaluator evaluator{ gc };


  auto obj = evaluator.evaluate(ast);

  // debug(
  //   std::cout
  //     << "-----------------------\n"
  //     << "evaluated result:\n\n"
  //     << obj->to_string() << std::endl;
  // );


  return 0;
}