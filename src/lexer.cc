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

  this->pass_space();

  while( this->check() ) {
    this->pass_space();

    auto& token = ret.emplace_back(TOK_End);

    auto ch = this->peek();
    auto str = this->source.data() + this->position;

    if( isdigit(ch) ) {
      token.str = { str, this->pass_while(isdigit) };
      continue;
    }
    else if( isalpha(ch) || ch == '_' ) {
      
    }

    for( std::string_view&& view : punctuaters ) {
      
    }
  }

  return ret;
}

bool Lexer::check() {
  return this->position < this->source.length();
}

char Lexer::peek() {
  return this->source[this->position];
}

bool Lexer::match(std::string_view str) {
  
}

size_t Lexer::pass_while(std::function<bool(char)> cond) {
  size_t len = 0;

  while( this->check() && cond(this->peek()) ) {
    len++;
    this->position++;
  }

  return len;
}


