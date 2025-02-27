#include "Utils.h"
#include "Debug.h"

#include "AST/AST.h"
#include "Parser.h"
#include "Error.h"

#include "ScriptFileContext.h"

Parser::Parser(ScriptFileContext& context, std::list<Token>& token_list)
  : in_impl(false),
    _context(context),
    _token_list(token_list)
{
  this->cur = this->_token_list.begin();
  this->ate = this->_token_list.end();
}

Parser::~Parser()
{
}

AST::Scope* Parser::parse()
{
  auto root_scope = new AST::Scope(*this->cur);

  while (this->check()) {
    if (this->eat("import")) {
      auto const& token = *this->ate;

      auto path = std::string(this->expect_identifier()->str);

      while (this->eat("/")) {
        path += "/" + std::string(this->expect_identifier()->str);
      }

      path += ".metro";

      if (!this->_context.import(path, token, root_scope)) {
        Error(token, "failed to import file '" + path + "'").emit().exit();
      }

      alertmsg(this->cur->str);

      continue;
    }

    auto x = root_scope->append(this->top());

    if (this->check() && !x->is_ended_with_scope())
      this->expect_semi();
    else
      this->eat_semi();
  }

  root_scope->end_token = --this->cur;

  return root_scope;
}

AST::Base* Parser::top()
{
  if (this->found("fn"))
    return this->parse_function();

  if (this->found("struct"))
    return this->parse_struct();

  if (this->found("enum"))
    return this->parse_enum();

  if (this->found("impl"))
    return this->parse_impl();

  return this->expr();
}

AST::Base* Parser::factor()
{
  if (this->eat("(")) {
    auto x = this->expr();
    this->expect(")");
    return x;
  }

  if (this->eat("none"))
    return this->set_last_token(new AST::ConstKeyword(AST_None, *this->ate));

  if (this->eat("true"))
    return this->set_last_token(new AST::ConstKeyword(AST_True, *this->ate));

  if (this->eat("false"))
    return this->set_last_token(new AST::ConstKeyword(AST_False, *this->ate));

  if (this->eat("{")) {
    if (auto tok = this->ate; this->eat("}"))
      return this->set_last_token(new AST::Scope(*tok));

    auto token = this->cur;

    auto x = this->expr();

    if (this->eat(":")) {
      auto ast = new AST::Dict(*token);

      ast->elements.emplace_back(*this->ate, x, this->expr());

      while (this->eat(",")) {
        ast->append(this->expr(), *this->expect(":"), this->expr());
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

      //
      // 変数
      return this->set_last_token(new AST::Variable(*ident));
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

AST::Base* Parser::indexref()
{
  auto x = this->primary();

  AST::IndexRef* ast = nullptr;

  if (this->found("[") || this->found(".")) {
    ast = new AST::IndexRef(x->token, x);
  }
  else
    return x;

  while (this->check()) {
    if (this->found("[")) {
      while (this->eat("[")) {
        ast->indexes.emplace_back(AST::IndexRef::Subscript::SUB_Index,
                                  this->expr());

        this->expect("]");
      }
    }
    else if (this->eat(".")) {
      auto m = this->primary();

      auto& sub =
        ast->indexes.emplace_back(AST::IndexRef::Subscript::SUB_Member, m);

      if (m->kind == AST_CallFunc)
        sub.kind = AST::IndexRef::Subscript::SUB_CallFunc;
      else if (m->kind != AST_Variable)
        Error(ERR_InvalidSyntax, m, "invalid syntax").emit().exit();
    }

    else
      break;
  }

  return this->set_last_token(ast);
}

AST::Base* Parser::unary()
{
  if (this->eat("-"))
    return new AST::UnaryOp(AST_UnaryMinus, *this->ate, this->indexref());

  if (this->eat("+"))
    return new AST::UnaryOp(AST_UnaryPlus, *this->ate, this->indexref());

  //
  // Type Constructor
  if (this->eat("new")) {
    auto ast = new AST::StructConstructor(*this->ate, this->expect_typename());

    this->expect("(");

    do {
      ast->init_pair_list.emplace_back(*this->expect_identifier(),
                                       *this->expect(":"), this->expr());
    } while (this->eat(","));

    this->expect(")");

    return ast;
  }

  return this->indexref();
}

AST::Base* Parser::mul()
{
  auto x = this->unary();

  while (this->check()) {
    if (this->eat("*"))
      AST::Expr::create(x)->append(AST::EX_Mul, *this->ate, this->unary());
    else if (this->eat("/"))
      AST::Expr::create(x)->append(AST::EX_Div, *this->ate, this->unary());
    else if (this->eat("%"))
      AST::Expr::create(x)->append(AST::EX_Mod, *this->ate, this->unary());
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
      AST::Expr::create(x)->append(AST::EX_Add, *this->ate, this->mul());
    else if (this->eat("-"))
      AST::Expr::create(x)->append(AST::EX_Sub, *this->ate, this->mul());
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
      AST::Expr::create(x)->append(AST::EX_LShift, *this->ate, this->add());
    else if (this->eat(">>"))
      AST::Expr::create(x)->append(AST::EX_RShift, *this->ate, this->add());
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
      AST::Compare::create(x)->append(AST::CMP_LeftBigOrEqual, *this->ate,
                                      this->shift());
    else if (this->eat("<="))
      AST::Compare::create(x)->append(AST::CMP_RightBigOrEqual, *this->ate,
                                      this->shift());
    else if (this->eat(">"))
      AST::Compare::create(x)->append(AST::CMP_LeftBigger, *this->ate,
                                      this->shift());
    else if (this->eat("<"))
      AST::Compare::create(x)->append(AST::CMP_RightBigger, *this->ate,
                                      this->shift());
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
      AST::Expr::create(x)->append(AST::EX_BitAND, *this->ate, this->compare());
    else if (this->eat("^"))
      AST::Expr::create(x)->append(AST::EX_BitXOR, *this->ate, this->compare());
    else if (this->eat("|"))
      AST::Expr::create(x)->append(AST::EX_BitOR, *this->ate, this->compare());
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
      AST::Expr::create(x)->append(AST::EX_And, *this->ate, this->compare());
    else if (this->eat("||"))
      AST::Expr::create(x)->append(AST::EX_Or, *this->ate, this->compare());
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

/**
 * @brief ステートメント
 *
 * @return AST::Base*
 */
AST::Base* Parser::stmt()
{
  //
  // Dict or Scope
  if (this->cur->str == "{")
    return this->parse_scope();

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
  // switch
  if (this->eat("switch")) {
    auto ast = new AST::Switch(*this->ate);

    ast->expr = this->expr();

    this->expect("{");

    while (this->eat("case")) {
      auto x = new AST::Case(*this->ate);

      x->cond = this->expr();

      this->expect(":");
      x->scope = this->expect_scope();

      x->end_token = x->scope->end_token;

      ast->cases.emplace_back(x);
    }

    ast->end_token = this->expect("}");

    if (ast->cases.empty()) {
      Error(ERR_EmptySwitch, ast->token, "empty switch-statement is not valid")
        .emit()
        .exit();
    }

    return ast;
  }

  //
  // match
  if (this->eat(""))

    //
    // loop
    if (this->eat("loop")) {
      return this->set_last_token(new AST::Loop(this->expect_scope()));
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
      ast->type = this->expect_typename();
    }

    if (this->eat("=")) {
      ast->init = this->expr();
    }

    return this->set_last_token(ast);
  }

  //
  // const
  if (this->eat("const")) {
    auto ast = new AST::VariableDeclaration(*this->ate);

    ast->name = this->expect_identifier()->str;

    if (this->eat(":")) {
      ast->type = this->expect_typename();
    }

    this->expect("=");

    ast->init = this->expr();

    ast->is_const = true;

    return this->set_last_token(ast);
  }

  //
  // return 文
  if (this->eat("return")) {
    auto ast = new AST::Return(*this->ate);

    if (this->eat_semi())
      return ast;

    ast->expr = this->expr();

    return this->set_last_token(ast);
  }

  // break
  if (this->eat("break")) {
    auto ast = new AST::LoopController(*this->ate, AST_Break);

    return this->set_last_token(ast);
  }

  // continue
  if (this->eat("continue")) {
    auto ast = new AST::LoopController(*this->ate, AST_Continue);

    return this->set_last_token(ast);
  }

  return nullptr;
}
