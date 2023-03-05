#include <cstring>

#include "Utils.h"
#include "debug/alert.h"

#include "Token.h"
#include "Lexer.h"
#include "Error.h"

#include "Application.h"
#include "ScriptFileContext.h"

static char const* punctuators[]{
    "->",

    "&&", "||",

    "<<", ">>",

    "..",

    "==", "!=", ">=", "<=", ">", "<",

    "!",  "?",

    "&",  "^",  "|",  "~",

    "=",  "+",  "-",  "*",  "/", "%",

    ",",  // comma
    ".",  // dot

    ";",  // semicolon
    ":",  // colon

    "(",  ")",  "[",  "]",  "{", "}", "<", ">",

};

std::string const& SourceLoc::get_source() const
{
  return this->context->get_source_code();
}

Lexer::Lexer(std::string const& source)
    : source(source),
      position(0)
{
}

Lexer::~Lexer()
{
}

//
// do lex !!
std::list<Token> Lexer::lex()
{
  std::list<Token> ret;

  this->pass_space();

  while (this->check()) {
    auto& token = ret.emplace_back(TOK_End);

    auto ch = this->peek();
    auto str = this->source.data() + this->position;

    token.src_loc.position = this->position;

    // digits
    if (isdigit(ch)) {
      token.kind = TOK_Int;
      token.str = {str, this->pass_while(isdigit)};

      if (this->peek() == 'u') {
        token.kind = TOK_USize;

        this->position++;
        token.str = {str,
                     this->position - token.src_loc.position};
      }
      else if (this->peek() == '.') {
        this->position++;

        if (isdigit(this->peek())) {
          this->pass_while(isdigit);
          token.kind = TOK_Float;
          token.str = {str,
                       this->position - token.src_loc.position};
        }
        else {
          this->position--;
        }
      }
    }

    // string
    else if (ch == '"') {
      token.kind = TOK_String;

      this->position++;
      str++;

      token.str = {str, this->pass_while([](char c) {
                     return c != '"';
                   })};

      this->position++;
    }

    // identifier
    else if (isalpha(ch) || ch == '_') {
      token.kind = TOK_Ident;

      token.str = {str, this->pass_while([](char c) {
                     return isalnum(c) || c == '_';
                   })};
    }

    else if (!this->find_punctuator(token)) {
    }

    // punctuator
    else if (std::all_of(
                 std::begin(punctuators), std::end(punctuators),
                 [&](char const*& s) {
                   if (this->match(s)) {
                     token.kind = TOK_Punctuater;

                     auto kind_offs = (&s - punctuators) /
                                      sizeof(char const*);

                     token.punct_kind =
                         static_cast<PunctuatorKind>(kind_offs);

                     if (token.punct_kind >= PU_Bracket) {
                       kind_offs -= PU_Bracket;

                       token.bracket_kind =
                           static_cast<BracketKind>(kind_offs /
                                                    2);

                       token.is_bracket_opened =
                           !(token.bracket_kind % 2);
                     }

                     token.str = s;
                     this->position += token.str.length();
                     return false;
                   }

                   return true;
                 })) {
      panic("unknown token at " << this->position);
    }

    token.src_loc.length = token.str.length();

    this->pass_space();
  }

  ret.emplace_back(TOK_End).src_loc.position =
      this->source.length() - 1;

  return ret;
}
