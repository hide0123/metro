#include <iostream>
#include <fstream>
#include "lcc.h"

int main(int argc, char** argv) {
  
  std::ifstream ifs{ "test.txt" };

  std::string source;
  
  for( std::string line; std::getline(ifs, line); )
    source += line;
  

  Lexer lexer{ source };

  auto const& token_list = lexer.lex();

  for( auto&& tok : token_list ) {
    std::cout << tok.str << std::endl;
  }

  Parser parser{ token_list };

  auto ast = parser.parse();


  return 0;
}