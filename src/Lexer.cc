#include "Utils.h"
#include "debug/alert.h"

#include "Token.h"
#include "Lexer.h"
#include "Error.h"

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

std::string& SourceLoc::get_source()
{
  return this->context->get_source_code();
}

Lexer::Lexer(ScriptFileContext& context)
    : source(context.get_source_code()),
      position(0),
      _context(context)
{
}

Lexer::~Lexer()
{
}

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

void Lexer::pass_space()
{
  auto pos = this->position;

  while (this->check()) {
    this->pass_while(isspace);

    if (this->match("//")) {
      this->position += 2;

      while (!this->match("\n")) {
        this->position++;
      }

      this->position++;
    }
    else if (this->match("/*")) {
      this->position += 2;

      while (!this->match("*/")) {
        this->position++;
      }

      this->position += 2;
    }
    else
      break;
  }

  while (pos < this->position) {
    this->source[pos++] = ' ';
  }
}

bool Lexer::find_punctuator(Token& token)
{
  size_t kind_offs = 0;

  for (auto&& s : punctuators) {
    if (this->match(s)) {
      token.kind = TOK_Punctuater;

      token.punct_kind =
          static_cast<PunctuatorKind>(kind_offs);

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
        token.str = {
            str, this->position - token.src_loc.position};
      }
      else if (this->peek() == '.') {
        this->position++;

        if (isdigit(this->peek())) {
          this->pass_while(isdigit);
          token.kind = TOK_Float;
          token.str = {
              str, this->position - token.src_loc.position};
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
                   }) + 2};

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
