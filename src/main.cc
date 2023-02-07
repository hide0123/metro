#include <iostream>
#include <fstream>
#include "lcc.h"


Application::Application() {

}

Application::~Application() {

}

void Application::initialize() {


  Object::initialize();


}

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

  for( auto&& tok : token_list ) {
    std::cout << tok.str << std::endl;
  }

  Parser parser{ token_list };

  auto ast = parser.parse();

  std::cout << ast->to_string() << std::endl;


  TypeChecker checker;

  auto type = checker.check(ast);

  std::cout << type.to_string() << std::endl;


  Evaluator evaluator;

  auto obj = evaluator.evaluate(ast);

  std::cout << obj->to_string() << std::endl;


  return 0;
}