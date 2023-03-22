#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Parser.h"
#include "Error.h"

AST::Base* Parser::factor()
{
  if (this->eat("(")) {
    auto x = this->expr();
    this->expect(")");
    return x;
  }

  if (this->eat("none"))
    return new AST::ConstKeyword(AST_None, *this->ate);

  if (this->eat("true"))
    return new AST::ConstKeyword(AST_True, *this->ate);

  if (this->eat("false"))
    return new AST::ConstKeyword(AST_False, *this->ate);

  if (this->eat("{")) {
    auto token = this->cur;

    auto x = this->expr();

    if (this->eat(":")) {
      auto ast = new AST::Dict(*token);

      ast->elements.emplace_back(*this->ate, x,
                                 this->expr());

      while (this->eat(",")) {
        ast->append(this->expr(), *this->expect(":"),
                    this->expr());
      }

      ast->end_token = this->expect("}");

      return ast;
    }

    return this->parse_scope(token, x);
  }

  if (auto x = this->stmt(); x)
    return x;

  //
  // トークンの種類ごとの処理
  switch (this->cur->kind) {
    // 即値・リテラル
    case TOK_Int:
    case TOK_USize:
    case TOK_Float:
    case TOK_Char:
    case TOK_String: {
      auto ast = new AST::Value(*this->cur);

      ast->end_token = this->next();

      return ast;
    }

    // 識別子
    case TOK_Ident: {
      // .str を取るために一時保存
      auto ident = this->cur;

      // 変数か関数呼び出しか 現時点でわからないので、
      // ひとつ先に進める
      this->next();

      // かっこ があれば 関数呼び出し
      if (this->eat("(")) {
        // AST 作成
        auto ast = new AST::CallFunc(*ident);

        // 引数をパースする
        if (!this->eat(")")) {
          do {
            // 式を追加
            ast->args.emplace_back(this->expr());
          } while (this->eat(","));  // カンマがある限り続く

          // 閉じかっこ
          ast->end_token = this->expect(")");
        }
        else {
          ast->end_token = this->ate;
        }

        return ast;
      }

      AST::Type* ast_type = nullptr;

      if (this->found("<")) {
        this->cur = ident;
        ast_type = this->expect_typename();
      }

      //
      // type constructor
      if (this->eat("{")) {
        if (!ast_type) {
          ast_type = new AST::Type(*ident);
        }

        auto ast = new AST::TypeConstructor(ast_type);

        do {
          ast->append(this->expr(), *this->expect(":"),
                      this->expr());
        } while (this->eat(","));

        ast->end_token = this->expect("}");

        return ast;
      }

      //
      // 変数
      return this->set_last_token(
          new AST::Variable(*ident));
    }
  }

  Error(*this->cur, "invalid syntax").emit().exit();
}

//
// primary
AST::Base* Parser::primary()
{
  // vector
  if (this->eat("[")) {
    auto ast = new AST::Vector(*this->ate);

    if (!this->eat("]")) {
      do {
        ast->append(this->expr());
      } while (this->eat(","));

      ast->end_token = this->expect("]");
    }
    else {
      ast->end_token = this->ate;
    }

    return ast;
  }

  if (this->eat("dict")) {
    auto ast = new AST::Dict(*this->ate);

    this->expect("<");

    ast->key_type = this->expect_typename();
    this->expect(",");

    ast->value_type = this->expect_typename();
    this->expect(">");

    this->expect("{");

    if (!this->eat("}")) {
      do {
        auto key = this->expr();

        auto colon = this->expect(":");

        auto value = this->expr();

        ast->elements.emplace_back(*colon, key, value);
      } while (this->eat(","));

      ast->end_token = this->expect("}");
    }
    else {
      ast->end_token = this->ate;
    }

    return ast;
  }

  //
  // Cast to any type
  if (this->eat("cast")) {
    auto ast = new AST::Cast(*this->ate);

    this->expect("<");
    ast->cast_to = this->expect_typename();
    this->expect(">");

    this->expect("(");
    ast->expr = this->expr();
    ast->end_token = this->expect(")");

    return ast;
  }

  return this->factor();
}

AST::Base* Parser::unary()
{
  if (this->eat("-"))
    return new AST::UnaryOp(AST_UnaryMinus, *this->ate,
                            this->primary());

  if (this->eat("+"))
    return new AST::UnaryOp(AST_UnaryPlus, *this->ate,
                            this->primary());

  return this->primary();
}

AST::Base* Parser::indexref()
{
  auto x = this->unary();

  AST::IndexRef* ast = nullptr;

  if (this->found("[") || this->found(".")) {
    ast = new AST::IndexRef(x->token, x);
  }
  else
    return x;

  while (this->check()) {
    alert;

    if (this->found("[")) {
      alert;

      while (this->eat("[")) {
        alert;

        ast->indexes.emplace_back(
            AST::IndexRef::Subscript::SUB_Index,
            this->expr());

        this->expect("]");
      }
    }
    else if (this->eat(".")) {
      alert;

      auto tmp = this->unary();

      //
      // 右辺に関数呼び出しがあった場合、
      // 式を置き換えて、第一引数に左辺を移動させる
      if (tmp->kind == AST_CallFunc) {
        // a.f()    -->  f(a)
        // a.b.f()  -->  f(a.b)

        auto cf = (AST::CallFunc*)tmp;

        // メンバ参照してなければ、最初の要素だけ追加
        // if (ast->indexes.empty()) {
        //   cf->args.insert(cf->args.begin(), ast->expr);

        //   ast->expr = nullptr;
        //   delete ast;
        // }
        // else  // あれば全体を追加
        cf->args.insert(cf->args.begin(), ast);

        if (!this->found(".")) {
          return cf;
        }
        else {
          ast = new AST::IndexRef(cf->token, cf);
        }

        continue;
      }

      ast->indexes.emplace_back(
          AST::IndexRef::Subscript::SUB_Member, tmp);
    }
    else
      break;
  }

  return this->set_last_token(ast);
}

AST::Base* Parser::mul()
{
  auto x = this->indexref();

  while (this->check()) {
    if (this->eat("*"))
      AST::Expr::create(x)->append(AST::EX_Mul, *this->ate,
                                   this->indexref());
    else if (this->eat("/"))
      AST::Expr::create(x)->append(AST::EX_Div, *this->ate,
                                   this->indexref());
    else if (this->eat("%"))
      AST::Expr::create(x)->append(AST::EX_Mod, *this->ate,
                                   this->indexref());
    else
      break;
  }

  return this->set_last_token(x);
}

AST::Base* Parser::add()
{
  auto x = this->mul();

  while (this->check()) {
    if (this->eat("+"))
      AST::Expr::create(x)->append(AST::EX_Add, *this->ate,
                                   this->mul());
    else if (this->eat("-"))
      AST::Expr::create(x)->append(AST::EX_Sub, *this->ate,
                                   this->mul());
    else
      break;
  }

  return this->set_last_token(x);
}

AST::Base* Parser::shift()
{
  auto x = this->add();

  while (this->check()) {
    if (this->eat("<<"))
      AST::Expr::create(x)->append(AST::EX_LShift,
                                   *this->ate, this->add());
    else if (this->eat(">>"))
      AST::Expr::create(x)->append(AST::EX_RShift,
                                   *this->ate, this->add());
    else
      break;
  }

  return this->set_last_token(x);
}

AST::Base* Parser::compare()
{
  auto x = this->shift();

  while (this->check()) {
    if (this->eat("=="))
      AST::Compare::create(x)->append(
          AST::CMP_Equal, *this->ate, this->shift());
    else if (this->eat("!="))
      AST::Compare::create(x)->append(
          AST::CMP_Equal, *this->ate, this->shift());
    else if (this->eat(">="))
      AST::Compare::create(x)->append(
          AST::CMP_LeftBigOrEqual, *this->ate,
          this->shift());
    else if (this->eat("<="))
      AST::Compare::create(x)->append(
          AST::CMP_RightBigOrEqual, *this->ate,
          this->shift());
    else if (this->eat(">"))
      AST::Compare::create(x)->append(
          AST::CMP_LeftBigger, *this->ate, this->shift());
    else if (this->eat("<"))
      AST::Compare::create(x)->append(
          AST::CMP_RightBigger, *this->ate, this->shift());
    else
      break;
  }

  return this->set_last_token(x);
}

AST::Base* Parser::bit_op()
{
  auto x = this->compare();

  while (this->check()) {
    if (this->eat("&"))
      AST::Expr::create(x)->append(
          AST::EX_BitAND, *this->ate, this->compare());
    else if (this->eat("^"))
      AST::Expr::create(x)->append(
          AST::EX_BitXOR, *this->ate, this->compare());
    else if (this->eat("|"))
      AST::Expr::create(x)->append(
          AST::EX_BitOR, *this->ate, this->compare());
    else
      break;
  }

  return this->set_last_token(x);
}

AST::Base* Parser::log_and_or()
{
  auto x = this->bit_op();

  while (this->check()) {
    if (this->eat("&&"))
      AST::Expr::create(x)->append(AST::EX_And, *this->ate,
                                   this->compare());
    else if (this->eat("||"))
      AST::Expr::create(x)->append(AST::EX_Or, *this->ate,
                                   this->compare());
    else
      break;
  }

  return this->set_last_token(x);
}

AST::Base* Parser::range()
{
  auto x = this->log_and_or();

  if (this->eat("..")) {
    auto y = new AST::Range(*this->ate);

    y->begin = x;
    y->end = this->log_and_or();

    x = y;
  }

  return this->set_last_token(x);
}

AST::Base* Parser::assign()
{
  auto x = this->range();

  if (this->eat("=")) {
    auto y = new AST::Assign(*this->ate);
    y->dest = x;
    y->expr = this->assign();
    x = y;
  }

  return this->set_last_token(x);
}

AST::Base* Parser::expr()
{
  return this->assign();
}
