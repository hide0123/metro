#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Parser.h"
#include "Error.h"

/**
 * @brief ステートメント
 *
 * @return AST::Base*
 */
AST::Base* Parser::stmt()
{
  /*
   最後がセミコロンで終わるやつは最後で
     this->expect_semi();
     　ってやってください
     例えば変数定義とか
  */

  //
  // スコープ
  if (this->cur->str == "{")
    return this->parse_scope();

  //
  // if 文
  if (this->eat("if")) {
    auto ast = new AST::If(*this->ate);

    ast->condition = this->expr();

    ast->if_true = this->expect_scope();

    if (this->eat("else")) {
      ast->if_false = this->stmt();
    }

    return ast;
  }

  //
  // for
  if (this->eat("for")) {
    auto ast = new AST::For(*this->ate);

    ast->iter = this->expr();

    this->expect("in");
    ast->iterable = this->expr();

    ast->code = this->expect_scope();

    return ast;
  }

  //
  // while
  if (this->eat("while")) {
    auto ast = new AST::While(*this->ate);
    ast->cond = this->expr();
    ast->code = this->expect_scope();
    return ast;
  }

  //
  // loop
  if (this->eat("loop")) {
    return new AST::Loop(this->expect_scope());
  }

  //
  // do-while
  if (this->eat("do")) {
    auto ast = new AST::DoWhile(*this->ate);

    ast->code = this->expect_scope();
    ast->cond = this->expr();

    this->expect_semi();
    return ast;
  }

  //
  // let 変数定義
  if (this->eat("let")) {
    auto ast = new AST::VariableDeclaration(*this->ate);

    ast->name = this->expect_identifier()->str;

    if (this->eat(":")) {
      ast->type = this->parse_typename();
    }

    if (this->eat("=")) {
      ast->init = this->expr();
    }

    this->expect_semi();
    return ast;
  }

  //
  // return 文
  if (this->eat("return")) {
    auto ast = new AST::Return(*this->ate);

    if (this->eat_semi())
      return ast;

    ast->expr = this->expr();
    this->expect_semi();

    return ast;
  }

  // break
  if (this->eat("break")) {
    auto ret = new AST::LoopController(*this->ate, AST_Break);

    this->expect_semi();

    return ret;
  }

  // continue
  if (this->eat("continue")) {
    auto ret = new AST::LoopController(*this->ate, AST_Continue);

    this->expect_semi();

    return ret;
  }

  auto ast = this->expr();

  this->expect_semi();

  return ast;
}

AST::Base* Parser::top()
{
  if (this->cur->str == "fn") {
    return this->parse_function();
  }

  return this->stmt();
}
