#include <fstream>

#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Lexer.h"
#include "Parser.h"
#include "Error.h"

#include "Application.h"

/**
 * @brief スコープをパースする
 *
 * @note 最後にセミコロンがない場合、最後の要素を return
 * 文にする
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
    Error(*(--this->cur), "expected identifier after this token")
        .emit()
        .exit();
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