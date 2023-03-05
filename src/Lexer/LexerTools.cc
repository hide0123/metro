#include <cstring>

#include "Utils.h"
#include "debug/alert.h"

#include "Token.h"
#include "Lexer.h"
#include "Error.h"

#include "Application.h"
#include "ScriptFileContext.h"

static char const* punctuators[]{
    // specify return type
    "->",

    // logical
    "&&",
    "||",

    // shift
    "<<",
    ">>",

    // range
    "..",

    // compare
    "==",
    "!=",
    ">=",
    "<=",
    ">",
    "<",

    "!",
    "?",

    // bit-calculation
    "&",
    "^",
    "|",
    "~",

    // assignment
    "=",

    // expr
    "+",
    "-",
    "*",
    "/",
    "%",

    ",",  // comma
    ".",  // dot

    ";",  // semicolon
    ":",  // colon

    // brackets
    "(",
    ")",
    "[",
    "]",
    "{",
    "}",
    "<",
    ">",
};

bool Lexer::check()
{
  return this->position < this->source.length();
}

char Lexer::peek()
{
  return this->source[this->position];
}

bool Lexer::match(std::string_view str)
{
  return this->position + str.length() <=
             this->source.length() &&
         std::memcmp(this->source.data() + this->position,
                     str.data(), str.length()) == 0;
}

size_t Lexer::pass_while(std::function<bool(char)> cond)
{
  size_t len = 0;

  while (this->check() && cond(this->peek())) {
    len++;
    this->position++;
  }

  return len;
}

size_t Lexer::pass_space()
{
  return this->pass_while(isspace);
}

bool Lexer::find_punctuator(Token& token)
{
  size_t kind_offs = 0;

  for (auto&& s : punctuators) {
    if (this->match(s)) {
      token.kind = TOK_Punctuater;

      token.punct_kind = static_cast<PunctuatorKind>(kind_offs);

      if (token.punct_kind >= PU_Bracket) {
        kind_offs -= PU_Bracket;

        token.bracket_kind =
            static_cast<BracketKind>(kind_offs / 2);

        token.is_bracket_opened = !(token.bracket_kind % 2);
      }

      token.str = s;
      this->position += token.str.length();

      return true;
    }

    kind_offs++;
  }

  return false;
}
