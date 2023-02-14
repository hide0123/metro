#pragma once

#include <string>

// ---------------------------------------------
//  Token
// ---------------------------------------------

enum TokenKind {
  TOK_Int,
  TOK_Float,
  TOK_Char,
  TOK_String,
  TOK_Ident,
  TOK_Punctuater,
  TOK_End
};

struct Token {
  TokenKind kind;
  
  std::string_view str;

  size_t pos;

  static Token const semi;

  explicit Token(TokenKind kind)
    : kind(kind),
      pos(0)
  {
  }
};
