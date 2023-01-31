#include "lcc.h"

static char const* punctuaters[] {

};

Lexer::Lexer(std::string const& source)
  : source(source),
    position(0)
{
}

Lexer::~Lexer() {

}

std::list<Token> Lexer::lex() {
  std::list<Token> ret;

  while( this->check() ) {
    auto& token = ret.emplace_back(TOK_End);

    auto ch = this->peek();

    if( isdigit(ch) ) {
      
    }
    else if( isalpha(ch) || ch == '_' ) {

    }
    else {
      
    }
  }

  return ret;
}

bool Lexer::check() {
  
}