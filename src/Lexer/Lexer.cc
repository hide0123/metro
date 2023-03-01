#include <cstring>

#include "common.h"

#include "Token.h"
#include "Lexer.h"
#include "Error.h"

#include "debug/alert.h"

static char const* punctuators[] {
  "->",
  
  "&&",
  "||",

  "<<",
  ">>",

  "..",

  "==",
  "!=",
  ">=",
  "<=",
  ">",
  "<",

  "!",
  "?",

  "&",
  "^",
  "|",
  "~",

  "=",
  "+",
  "-",
  "*",
  "/",
  "%",

  ",", // comma
  ".", // dot

  ";", // semicolon
  ":", // colon
  
  "(",
  ")",
  "[",
  "]",
  "{",
  "}",
  "<",
  ">",
  

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
      token.kind = TOK_Int;
      token.str = { str, this->pass_while(isdigit) };

      if(this->peek()=='u'){
        token.kind=TOK_USize;

        this->position++;
        token.str={str,this->position-token.pos};
      }
    }

    // string
    else if( ch == '"' ) {
      token.kind = TOK_String;

      this->position++;
      str++;

      token.str = { str,
        this->pass_while([] (char c) { return c != '"'; }) };
      
      this->position++;
    }

    // identifier
    else if( isalpha(ch) || ch == '_' ) {
      token.kind = TOK_Ident;

      token.str = { str,
        this->pass_while(
          [] (char c) { return isalnum(c) || c == '_'; }) };
    }

    // punctuator
    else if( std::all_of(
          std::begin(punctuators),
          std::end(punctuators),
          [&] (char const*& s) {
            if( this->match(s) ) {
              token.kind = TOK_Punctuater;

              auto kind_offs = (&s - punctuators) / sizeof(char const*);

              token.punct_kind =
                static_cast<PunctuatorKind>(kind_offs);

              if( token.punct_kind >= PU_Bracket ) {
                kind_offs -= PU_Bracket;

                token.bracket_kind =
                  static_cast<BracketKind>(kind_offs / 2);
                
                token.is_bracket_opened =
                  !(token.bracket_kind % 2);
              }

              token.str = s;
              this->position += token.str.length();
              return false;
            }

            return true;
          }) ) {
      
      panic("unknown token at " << this->position);
    }

    this->pass_space();
  }

  ret.emplace_back(TOK_End);

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


