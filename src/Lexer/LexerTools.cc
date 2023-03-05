#include <cstring>

#include "Utils.h"
#include "debug/alert.h"

#include "Token.h"
#include "Lexer.h"
#include "Error.h"

#include "Application.h"
#include "ScriptFileContext.h"

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
