#include "metro.h"

/**
 * @brief 型名をパースする
 * 
 * @return AST::Type* 
 */
AST::Type* Parser::parse_typename() {
  auto ast =
    new AST::Type(*this->expect_identifier());

  // todo: 修飾子を読み取る

  return ast;
}

/**
 * @brief 型名を求める
 * 
 * @return AST::Type* ない場合はエラー
 */
AST::Type* Parser::expect_typename() {
  if( auto ast = this->parse_typename(); ast )
    return ast;

  Error(*({ auto tok = this->cur; --tok; }),
    "expected typename after this token"
  )
    .emit()
    .exit();
}

/**
 * @brief トークンが終端に来ていないか確認する
 * 
 * @return 終端でなければ true
 */
bool Parser::check() {
  return this->cur->kind != TOK_End;
}

/**
 * @brief トークンをひとつ先に進める
 * 
 */
void Parser::next() {
  this->cur++;
}

/**
 * @brief トークン消費
 * 
 * @param s: 文字列
 * @return s を食べたら true, そうでなければ false
 */
bool Parser::eat(char const* s) {
  if( this->cur->str == s ) {
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

// セミコロン消費
bool Parser::eat_semi() {
  return this->eat(";");
}

// セミコロン求める
Parser::token_iter Parser::expect_semi() {
  return this->expect(";");
}

// 識別子を求める
Parser::token_iter Parser::expect_identifier() {
  if( this->cur->kind != TOK_Ident ) {
    Error(*this->cur, "expected identifier")
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
AST::Return* Parser::new_return_stmt(AST::Base* ast) {
  auto ret = new AST::Return(ast->token);

  ret->expr = ast;

  return ret;
}