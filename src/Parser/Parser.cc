#include <fstream>

#include "common.h"

#include "AST.h"
#include "Parser.h"

#include "Lexer.h"

#include "Error.h"

#include "debug/alert.h"

using EXKind = AST::Expr::ExprKind;

std::string open_file(std::string const& path);

Parser::Parser(std::list<Token>&& token_list)
    : token_list(std::move(token_list))
{
  this->cur = this->token_list.begin();
  this->ate = this->token_list.end();
}

Parser::~Parser()
{
}

AST::Scope* Parser::parse()
{
  auto root_scope = new AST::Scope(*this->cur);

  while (this->check()) {
    if (this->eat("import")) {
      auto tok = this->ate;

      std::string path;

      do {
        path += this->expect_identifier()->str;
      } while (this->eat("/"));

      path += ".metro";

      std::ifstream ifs{path};

      if (ifs.fail()) {
        Error(*tok, "cannot open script file '" + path + "'")
            .emit()
            .exit();
      }

      auto source = new std::string;

      for (std::string line; std::getline(ifs, line);) {
        source->append(line + '\n');
      }

      auto _lexer = new Lexer(*source);

      auto _parser = new Parser(_lexer->lex());

      auto _imported = _parser->parse();

      for (auto&& x : _imported->list) {
        root_scope->append(x);
      }

      continue;
    }

    root_scope->append(this->top());
  }

  return root_scope;
}

//
// primary
AST::Base* Parser::primary()
{
  if (this->eat("[")) {
    auto ast = new AST::Array(*this->ate);

    if (!this->eat("]")) {
      do {
        ast->append(this->expr());
      } while (this->eat(","));

      this->expect("]");
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

      this->expect("}");
    }

    return ast;
  }

  if (this->eat("none"))
    return new AST::ConstKeyword(AST_None, *this->ate);

  if (this->eat("true"))
    return new AST::ConstKeyword(AST_True, *this->ate);

  if (this->eat("false"))
    return new AST::ConstKeyword(AST_False, *this->ate);

  if (this->eat("cast")) {
    auto ast = new AST::Cast(*this->ate);

    this->expect("<");
    ast->cast_to = this->expect_typename();
    this->expect(">");

    this->expect("(");
    ast->expr = this->expr();
    this->expect(")");

    return ast;
  }

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

      this->next();

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
        auto callFunc = new AST::CallFunc(*ident);

        // 引数をパースする
        if (!this->eat(")")) {
          do {
            // 式を追加
            callFunc->args.emplace_back(this->expr());
          } while (this->eat(","));  // カンマがある限り続く

          // 閉じかっこ
          this->expect(")");
        }

        return callFunc;
      }

      // 開きかっこがなければ 変数
      return new AST::Variable(*ident);
    }

    default:
      Error(*this->cur, "invalid syntax").emit().exit();
  }

  return this->stmt();
}

AST::Base* Parser::unary()
{
  if (this->eat("-"))
    return new AST::UnaryOp(AST_UnaryMinus, *this->ate,
                            this->expr());

  if (this->eat("+"))
    return new AST::UnaryOp(AST_UnaryPlus, *this->ate,
                            this->expr());

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

  return x;
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

  return x;
}

AST::Base* Parser::mul()
{
  auto x = this->member_access();

  while (this->check()) {
    if (this->eat("*"))
      AST::Expr::create(x)->append(EXKind::EX_Mul, *this->ate,
                                   this->member_access());
    else if (this->eat("/"))
      AST::Expr::create(x)->append(EXKind::EX_Div, *this->ate,
                                   this->member_access());
    else if (this->eat("%"))
      AST::Expr::create(x)->append(EXKind::EX_Mod, *this->ate,
                                   this->member_access());
    else
      break;
  }

  return x;
}

AST::Base* Parser::add()
{
  auto x = this->mul();

  while (this->check()) {
    if (this->eat("+"))
      AST::Expr::create(x)->append(EXKind::EX_Add, *this->ate,
                                   this->mul());
    else if (this->eat("-"))
      AST::Expr::create(x)->append(EXKind::EX_Sub, *this->ate,
                                   this->mul());
    else
      break;
  }

  return x;
}

AST::Base* Parser::shift()
{
  auto x = this->add();

  while (this->check()) {
    if (this->eat("<<"))
      AST::Expr::create(x)->append(EXKind::EX_LShift, *this->ate,
                                   this->add());
    else if (this->eat(">>"))
      AST::Expr::create(x)->append(EXKind::EX_RShift, *this->ate,
                                   this->add());
    else
      break;
  }

  return x;
}

AST::Base* Parser::compare()
{
  auto x = this->shift();

  while (this->check()) {
    if (this->eat("=="))
      AST::Compare::create(x)->append(AST::Compare::CMP_Equal,
                                      *this->ate, this->shift());
    else if (this->eat("!="))
      AST::Compare::create(x)->append(AST::Compare::CMP_Equal,
                                      *this->ate, this->shift());
    else if (this->eat(">="))
      AST::Compare::create(x)->append(
          AST::Compare::CMP_LeftBigOrEqual, *this->ate,
          this->shift());
    else if (this->eat("<="))
      AST::Compare::create(x)->append(
          AST::Compare::CMP_RightBigOrEqual, *this->ate,
          this->shift());
    else if (this->eat(">"))
      AST::Compare::create(x)->append(
          AST::Compare::CMP_LeftBigger, *this->ate,
          this->shift());
    else if (this->eat("<"))
      AST::Compare::create(x)->append(
          AST::Compare::CMP_RightBigger, *this->ate,
          this->shift());
    else
      break;
  }

  return x;
}

AST::Base* Parser::bit_op()
{
  auto x = this->compare();

  while (this->check()) {
    if (this->eat("&"))
      AST::Expr::create(x)->append(EXKind::EX_BitAND, *this->ate,
                                   this->compare());
    else if (this->eat("^"))
      AST::Expr::create(x)->append(EXKind::EX_BitXOR, *this->ate,
                                   this->compare());
    else if (this->eat("|"))
      AST::Expr::create(x)->append(EXKind::EX_BitOR, *this->ate,
                                   this->compare());
    else
      break;
  }

  return x;
}

AST::Base* Parser::log_and_or()
{
  auto x = this->bit_op();

  while (this->check()) {
    if (this->eat("&&"))
      AST::Expr::create(x)->append(EXKind::EX_And, *this->ate,
                                   this->compare());
    else if (this->eat("||"))
      AST::Expr::create(x)->append(EXKind::EX_Or, *this->ate,
                                   this->compare());
    else
      break;
  }

  return x;
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

  return x;
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

  return x;
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

/**
 * @brief スコープをパースする
 *
 * @note 最後にセミコロンがない場合、最後の要素を return 文にする
 *
 * @return AST::Scope*
 */
AST::Scope* Parser::parse_scope()
{
  auto ast = new AST::Scope(*this->expect("{"));

  while (this->check() && !this->eat("}")) {
    ast->append(this->stmt());
  }

  return ast;
}

/**
 * @brief スコープを求める
 *
 * @return
 *   スコープを読み取れた場合は AST::Scope*
 *   なければエラー
 */
AST::Scope* Parser::expect_scope()
{
  if (auto ast = this->parse_scope(); ast) {
    return ast;
  }

  Error(*({
    auto tok = this->cur;
    --tok;
  }),
        "expected scope after this token")
      .emit()
      .exit();
}

/**
 * @brief 関数をパースする
 *
 * @note 必ず "fn" トークンがある位置で呼び出すこと
 *
 * @return AST::Function*
 */
AST::Function* Parser::parse_function()
{
  auto func =
      new AST::Function(*this->expect("fn"),
                        *this->expect_identifier());  // AST 作成

  this->expect("(");  // 引数リストの開きカッコ

  // 閉じかっこがなければ、引数を読み取っていく
  if (!this->eat(")")) {
    do {
      auto arg_name_token =
          this->expect_identifier();  // 引数名のトークン

      this->expect(":");  // コロン

      // 型
      func->append_argument(*arg_name_token,
                            this->expect_typename());
    } while (this->eat(","));  // カンマがあれば続ける

    this->expect(")");  // 閉じかっこ
  }

  // 戻り値の型
  if (this->eat("->")) {
    func->result_type = this->expect_typename();
  }

  func->code = this->expect_scope();  // 処理 (スコープ)

  return func;
}

/**
 * @brief 型名をパースする
 *
 * @return AST::Type*
 */
AST::Type* Parser::parse_typename()
{
  auto ast = new AST::Type(*this->expect_identifier());

  if (this->eat("<")) {
    do {
      ast->parameters.emplace_back(this->expect_typename());
    } while (this->eat(","));

    if (this->cur->str == ">>") {
      this->cur->str = ">";
    }
    else
      this->expect(">");
  }

  if (this->eat("const")) {
    ast->is_const = true;
  }

  return ast;
}

/**
 * @brief 型名を求める
 *
 * @return AST::Type* ない場合はエラー
 */
AST::Type* Parser::expect_typename()
{
  if (auto ast = this->parse_typename(); ast)
    return ast;

  Error(*({
    auto tok = this->cur;
    --tok;
  }),
        "expected typename after this token")
      .emit()
      .exit();
}

/**
 * @brief トークンが終端に来ていないか確認する
 *
 * @return 終端でなければ true
 */
bool Parser::check()
{
  return this->cur->kind != TOK_End;
}

/**
 * @brief トークンをひとつ先に進める
 *
 */
void Parser::next()
{
  this->cur++;
}

/**
 * @brief トークン消費
 *
 * @param s: 文字列
 * @return s を食べたら true, そうでなければ false
 */
bool Parser::eat(char const* s)
{
  if (this->cur->str == s) {
    this->ate = this->cur++;
    return true;
  }

  return false;
}

/**
 * @brief 指定したトークンを求める
 *
 * @param s: 文字列
 * @return
 *  食べたらそのトークンへのイテレータ (Parser::token_iter)
 *  を返し、無ければエラーで終了
 */
Parser::token_iter Parser::expect(char const* s)
{
  if (!this->eat(s)) {
    Error(*(--this->cur),
          "expected '" + std::string(s) + "' after this token")
        .emit()
        .exit();
  }

  return this->ate;
}

// セミコロン消費
bool Parser::eat_semi()
{
  return this->eat(";");
}

// セミコロン求める
Parser::token_iter Parser::expect_semi()
{
  return this->expect(";");
}

// 識別子を求める
Parser::token_iter Parser::expect_identifier()
{
  if (this->cur->kind != TOK_Ident) {
    Error(*this->cur, "expected identifier").emit().exit();
  }

  this->ate = this->cur++;
  return this->ate;
}

/**
 * @brief ast を返り値とする return 文を作成する
 *
 * @param ast: 返り値にさせる式
 * @return AST::Return*
 */
AST::Return* Parser::new_return_stmt(AST::Base* ast)
{
  auto ret = new AST::Return(ast->token);

  ret->expr = ast;

  return ret;
}