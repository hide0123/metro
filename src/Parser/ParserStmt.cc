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
  // Dict or Scope
  if (this->eat("{")) {
    auto token = this->ate;

    if (this->eat("}"))
      return new AST::Scope(*token);

    auto x = this->expr();

    // Dictionary
    if (this->eat(":")) {
      auto ast = new AST::Dict(*token);

      ast->elements.emplace_back(*this->ate, x, this->expr());

      while (this->eat(",")) {
        auto key = this->expr();
        auto colon = this->expect(":");

        auto value = this->expr();

        ast->elements.emplace_back(*colon, key, value);
      }

      ast->end_token = this->expect("}");

      return ast;
    }

    // Scope
    auto ast = new AST::Scope(*token);

    ast->append(x);

    while (this->check()) {
      auto prev = this->cur;
      prev--;

      if (prev->str == ";") {
        if (this->eat("}")) {
          break;
        }

        ast->append(this->expr());
        continue;
      }

      if (prev->str == "}") {
        if (this->eat("}")) {
          ast->return_last_expr = true;
          break;
        }

        ast->append(this->expr());
        continue;
      }

      if (auto bval = this->eat_semi(); this->eat("}")) {
        ast->return_last_expr = !bval;
        break;
      }
      else if (bval) {
        ast->append(this->expr());
      }
      else {
        this->expect("}");
        break;
      }
    }

    return ast;
  }

  //
  // if 文
  if (this->eat("if")) {
    auto ast = new AST::If(*this->ate);

    ast->condition = this->expr();

    ast->if_true = this->expect_scope();

    if (this->eat("else")) {
      if (this->cur->str == "if")
        ast->if_false = this->stmt();
      else
        ast->if_false = this->expect_scope();
    }

    return this->set_last_token(ast);
  }

  //
  // for
  if (this->eat("for")) {
    auto ast = new AST::For(*this->ate);

    ast->iter = this->expr();

    this->expect("in");
    ast->iterable = this->expr();

    ast->code = this->expect_scope();

    return this->set_last_token(ast);
  }

  //
  // while
  if (this->eat("while")) {
    auto ast = new AST::While(*this->ate);

    ast->cond = this->expr();
    ast->code = this->expect_scope();

    return this->set_last_token(ast);
  }

  //
  // loop
  if (this->eat("loop")) {
    return this->set_last_token(
        new AST::Loop(this->expect_scope()));
  }

  //
  // do-while
  if (this->eat("do")) {
    auto ast = new AST::DoWhile(*this->ate);

    ast->code = this->expect_scope();

    this->expect("while");
    ast->cond = this->expr();

    this->expect_semi();

    return this->set_last_token(ast);
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

    ast->end_token = this->expect_semi();

    return ast;
  }

  //
  // return 文
  if (this->eat("return")) {
    auto ast = new AST::Return(*this->ate);

    if (this->eat_semi())
      return ast;

    ast->expr = this->expr();
    ast->end_token = this->expect_semi();

    return ast;
  }

  // break
  if (this->eat("break")) {
    auto ast = new AST::LoopController(*this->ate, AST_Break);

    ast->end_token = this->expect_semi();

    return ast;
  }

  // continue
  if (this->eat("continue")) {
    auto ast = new AST::LoopController(*this->ate, AST_Continue);

    ast->end_token = this->expect_semi();

    return ast;
  }

  return nullptr;
}
