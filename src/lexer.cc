#include <cstring>
#include "lcc.h"

static char const* punctuaters[] {
  "==",
  "!=",
  "<=",
  ">=",
  "<<",
  ">>",
  "<",
  ">",

  "+",
  "-",
  "*",
  "/",
  
  "(",
  ")",
  "{",
  "}",
  "[",
  "]",
  
  ",",
  ".",
  ";",
  ":",

};

Lexer::Lexer(std::string const& source)
  : source(source),
    position(0)
{
}

Lexer::~Lexer() {

}

//
// do lex !!
std::list<Token> Lexer::lex() {
  std::list<Token> ret;

  this->pass_space();

  while( this->check() ) {
    auto& token = ret.emplace_back(TOK_End);

    auto ch = this->peek();
    auto str = this->source.data() + this->position;

    token.pos = this->position;

    // digits
    if( isdigit(ch) ) {
      token.str = { str, this->pass_while(isdigit) };
    }

    // identifier
    else if( isalpha(ch) || ch == '_' ) {
      token.str = { str,
        this->pass_while(
          [] (char c) { return isalnum(c) || c == '_'; }) };
    }

    // punctuater
    else if( std::all_of(
          std::begin(punctuaters),
          std::end(punctuaters),
          [&] (std::string_view&& s) {
            if( this->match(s) ) {
              token.str = s;
              this->position += s.length();
              return false;
            }

            return true;
          }) ) {
      
      panic("unknown token at %lld", this->position);
    }

    this->pass_space();
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
  return
    this->position + str.length() <= this->source.length()
    && std::memcmp(
        this->source.data() + this->position,
        str.data(), str.length()) == 0;
}

size_t Lexer::pass_while(std::function<bool(char)> cond) {
  size_t len = 0;

  while( this->check() && cond(this->peek()) ) {
    len++;
    this->position++;
  }

  return len;
}


