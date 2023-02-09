#include "lcc.h"

Parser::Parser(std::list<Token> const& token_list)
  : token_list(token_list),
    cur(token_list.begin())
{
}

Parser::~Parser() {

}

AST::Base* Parser::parse() {
  return this->expr();
}

AST::Base* Parser::primary() {
  switch( this->cur->kind ) {
    case TOK_Int:
    case TOK_Float:
    case TOK_Char:
    case TOK_String:
    {
      auto ast = new AST::Value(*this->cur);

      this->next();

      return ast;
    }

    case TOK_Ident: {
      // .str を取るために一時保存
      auto ident = this->cur;

      // 変数か関数呼び出しか 現時点でわからないので、
      // ひとつ先に進める
      this->next();

      // かっこ があれば 関数呼び出し
      if( this->eat("()") ) {
        auto callFunc = new AST::CallFunc(
      }

      // なければ 変数
      return new AST::Variable(*this->cur);
    }
  }

  alert;
  viewvar("%d", this->cur->kind);

  Error(*this->cur, "invalid syntax")
    .emit()
    .exit();
}

AST::Base* Parser::term() {
  auto x = this->primary();

  while( this->check() ) {
    if( this->eat("*") )
      AST::Expr::create(x)->append(AST::Expr::EX_Mul, *this->ate, this->primary());
    else if( this->eat("/") )
      AST::Expr::create(x)->append(AST::Expr::EX_Div, *this->ate, this->primary());
    else
      break;
  }

  return x;
}

AST::Base* Parser::expr() {
  auto x = this->term();

  while( this->check() ) {
    if( this->eat("+") )
      AST::Expr::create(x)->append(AST::Expr::EX_Add, *this->ate, this->term());
    else if( this->eat("-") )
      AST::Expr::create(x)->append(AST::Expr::EX_Sub, *this->ate, this->term());
    else
      break;
  }

  return x;
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