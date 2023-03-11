#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Parser.h"
#include "Error.h"

AST::Base* Parser::factor()
{
  if (this->eat("none"))
    return new AST::ConstKeyword(AST_None, *this->ate);

  if (this->eat("true"))
    return new AST::ConstKeyword(AST_True, *this->ate);

  if (this->eat("false"))
    return new AST::ConstKeyword(AST_False, *this->ate);

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

      // 開きかっこがなければ 変数
      auto ast = new AST::Variable(*ident);

      ast->end_token = ident;

      return ast;
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

  if (this->cur->str == "[") {
    auto y = new AST::IndexRef(*this->cur);

    y->expr = x;

    while (this->eat("[")) {
      y->indexes.emplace_back(this->expr());
      this->expect("]");
    }

    x = y;
  }

  return this->set_last_token(x);
}

AST::Base* Parser::member_access()
{
  auto x = this->indexref();

  if (this->cur->str == ".") {
    auto y = new AST::IndexRef(*this->cur);

    y->kind = AST_MemberAccess;
    y->expr = x;

    while (this->eat(".")) {
      auto tmp = this->indexref();

      if (tmp->kind == AST_CallFunc) {
        alert;
        auto cf = (AST::CallFunc*)tmp;

        if (y->indexes.empty())
          cf->args.insert(cf->args.begin(), y->expr);
        else
          cf->args.insert(cf->args.begin(), y);

        if (this->cur->str == ".") {
          alert;
          y = new AST::IndexRef(*this->cur);
          y->kind = AST_MemberAccess;
          y->expr = cf;
        }
        else {
          alert;
          return cf;
        }
      }
      else {
        alert;
        y->indexes.emplace_back(this->indexref());
      }
    }

    x = y;
  }

  return this->set_last_token(x);
}

AST::Base* Parser::mul()
{
  auto x = this->member_access();

  while (this->check()) {
    if (this->eat("*"))
      AST::Expr::create(x)->append(AST::EX_Mul, *this->ate,
                                   this->member_access());
    else if (this->eat("/"))
      AST::Expr::create(x)->append(AST::EX_Div, *this->ate,
                                   this->member_access());
    else if (this->eat("%"))
      AST::Expr::create(x)->append(AST::EX_Mod, *this->ate,
                                   this->member_access());
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
      AST::Expr::create(x)->append(AST::EX_LShift, *this->ate,
                                   this->add());
    else if (this->eat(">>"))
      AST::Expr::create(x)->append(AST::EX_RShift, *this->ate,
                                   this->add());
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
      AST::Compare::create(x)->append(AST::CMP_Equal, *this->ate,
                                      this->shift());
    else if (this->eat("!="))
      AST::Compare::create(x)->append(AST::CMP_Equal, *this->ate,
                                      this->shift());
    else if (this->eat(">="))
      AST::Compare::create(x)->append(AST::CMP_LeftBigOrEqual,
                                      *this->ate, this->shift());
    else if (this->eat("<="))
      AST::Compare::create(x)->append(AST::CMP_RightBigOrEqual,
                                      *this->ate, this->shift());
    else if (this->eat(">"))
      AST::Compare::create(x)->append(AST::CMP_LeftBigger,
                                      *this->ate, this->shift());
    else if (this->eat("<"))
      AST::Compare::create(x)->append(AST::CMP_RightBigger,
                                      *this->ate, this->shift());
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
      AST::Expr::create(x)->append(AST::EX_BitAND, *this->ate,
                                   this->compare());
    else if (this->eat("^"))
      AST::Expr::create(x)->append(AST::EX_BitXOR, *this->ate,
                                   this->compare());
    else if (this->eat("|"))
      AST::Expr::create(x)->append(AST::EX_BitOR, *this->ate,
                                   this->compare());
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
