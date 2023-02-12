#include "metro.h"

AST::Type* Parser::parse_typename() {
  auto ast =
    new AST::Type(*this->expect_identifier());

  // todo: 修飾子を読み取る

  return ast;
}

AST::Type* Parser::expect_typename() {
  if( auto ast = this->parse_typename(); ast )
    return ast;

  Error(*({ auto tok = this->cur; --tok; }),
    "expected typename after this token"
  )
    .emit()
    .exit();
}

bool Parser::check() {
  return this->cur->kind != TOK_End;
}

void Parser::next() {
  this->cur++;
}

bool Parser::eat(char const* s) {
  if( this->cur->str == s ) {
    this->ate = this->cur++;
    return true;
  }

  return false;
}

Parser::token_iter Parser::expect(char const* s) {
  if( !this->eat(s) ) {
    Error(*this->cur,
      "expected '" + std::string(s) + "' but found '"
        + std::string(this->cur->str) + "'")
      .emit()
      .exit();
  }

  return this->ate;
}

bool Parser::eat_semi() {
  return this->eat(";");
}

Parser::token_iter Parser::expect_semi() {
  return this->expect(";");
}

Parser::token_iter Parser::expect_identifier() {
  if( this->cur->kind != TOK_Ident ) {
    Error(*this->cur, "expected identifier")
      .emit()
      .exit();
  }

  this->ate = this->cur++;
  return this->ate;
}

AST::Return* Parser::new_return_stmt(AST::Base* ast) {
  auto ret = new AST::Return(ast->token);

  ret->expr = ast;

  return ret;
}