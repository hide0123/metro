#pragma once

#include <string>

// ---------------------------------------------
//  Token
// ---------------------------------------------

/**
 *
 * kind mem map
 *
 * uint32_t:
 *  AABBCCDD
 *
 *
 *  AA = token
 *  BB = punctuater kind
 *  CC = bracket kind
 *  DD = if bracket, is that opened or not (open = 1)
 */

enum TokenKind : uint8_t {
  TOK_Int,
  TOK_USize,
  TOK_Float,
  TOK_Char,
  TOK_String,
  TOK_Ident,
  TOK_Punctuater,
  TOK_End
};

enum PunctuatorKind : uint8_t {
  PU_SpecifyReturnType,

  PU_And,
  PU_Or,

  PU_LShift,
  PU_RShift,

  PU_Equal,
  PU_NotEqual,

  PU_LeftBigOrEqual,
  PU_RightBigOrEqual,

  PU_LeftBigger,
  PU_RightBigger,

  PU_Exclamation,
  PU_Question,

  PU_BitAND,
  PU_BitXOR,
  PU_BitOR,
  PU_BitNOT,

  PU_Assign,
  PU_Add,
  PU_Sub,
  PU_Mul,
  PU_Div,
  PU_Mod,

  PU_Comma,
  PU_Period,

  PU_Semicolon,
  PU_Colon,

  PU_Bracket,
};

enum BracketKind : uint8_t {
  BR_Round,  // ()
  BR_Square,  // []
  BR_Curly,  // {}
  BR_Angle  // <>
};

struct Token {
  union {
    uint32_t _kind;

    struct {
      TokenKind kind;
      PunctuatorKind punct_kind;
      BracketKind bracket_kind;
      bool is_bracket_opened : 8;
    };
  };

  std::string_view str;

  size_t pos;

  static Token const semi;

  explicit Token(TokenKind kind)
      : _kind(0),
        pos(0)
  {
    this->kind = kind;
  }
};
