#pragma once

#include <string>
#include <vector>
#include <list>

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
  // Token* prev;
  // Token* next;

  Token(TokenKind kind)
    : kind(TOK_Int)
  {
  }
};

enum ASTKind {
  AST_Value,
  AST_Variable,

  AST_Add,
  AST_Sub,

  AST_Function
};

namespace AST {

struct Base {
  ASTKind kind;
  Token const& token;
};

}

class Lexer {
public:
  Lexer(std::string const& source);
  ~Lexer();

  std::list<Token> lex();


private:

  std::string const& source;
  size_t position;
};

class Parser {

};

