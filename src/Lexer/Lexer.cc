#include "Utils.h"
#include "debug/alert.h"

#include "Token.h"
#include "Lexer.h"
#include "Error.h"

#include "ScriptFileContext.h"

Lexer::Lexer(ScriptFileContext const& context)
    : source(context.get_source_code()),
      position(0),
      _context(context)
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

  auto line_range = this->_context._srcdata._lines.begin();

  this->pass_space();

  while (this->check()) {
    auto& token = ret.emplace_back(TOK_End);

    auto ch = this->peek();
    auto str = this->source.data() + this->position;

    while (line_range->end <= this->position)
      line_range++;

    token.src_loc.context = &this->_context;
    token.src_loc.position = this->position;
    token.src_loc.line_num = line_range->index + 1;

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

      token.str = {str, this->pass_while([](char c) {
                     return c != '"';
                   }) + 1};

      this->position++;
    }

    // identifier
    else if (isalpha(ch) || ch == '_') {
      token.kind = TOK_Ident;

      token.str = {str, this->pass_while([](char c) {
                     return isalnum(c) || c == '_';
                   })};
    }

    // punctuator
    else if (!this->find_punctuator(token)) {
      token.str = " ";
      Error(token, "unknown token").emit().exit();
    }

    token.src_loc.length = token.str.length();

    this->pass_space();
  }

  ret.emplace_back(TOK_End).src_loc.position =
      this->source.length() - 1;

  return ret;
}
