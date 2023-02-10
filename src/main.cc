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

//
// テキストファイルを開く
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

  debug(
    for( auto&& tok : token_list ) {
      std::cout << tok.str << std::endl;
    }
  )

  Parser parser{ token_list };

  auto ast = parser.parse();

  debug(
    std::cout << ast->to_string() << std::endl;
  )


  Checker checker;

  auto type = checker.check(ast);

  debug(
    std::cout << type.to_string() << std::endl;
  )

  GarbageCollector gc;

  Evaluator evaluator{ gc };


  auto obj = evaluator.evaluate(ast);

  std::cout << obj->to_string() << std::endl;


  return 0;
}