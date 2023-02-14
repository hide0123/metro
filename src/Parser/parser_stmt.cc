#include "metro.h"

AST::Base* Parser::top() {
  if( this->cur->str == "fn" ) {
    return this->parse_function();
  }

  return this->expr();
}

/**
 * @brief スコープをパースする
 * 
 * @note 最後にセミコロンがない場合、最後の要素を return 文にする
 * 
 * @return AST::Scope* 
 */
AST::Scope* Parser::parse_scope() {
  auto ast =
    new AST::Scope(*this->expect("{"));
  
  // 空だったら帰る
  if( this->eat("}") )
    return ast;

  while( this->check() ) {
    auto& item = ast->append(this->expr());

    if( this->eat_semi() ) {
      if( !this->eat("}") )
        return ast;
      
      continue;
    }

    this->expect("}");
    break;
  }

  this->to_return_stmt(*ast->list.rbegin());

  return ast;
}

/**
 * @brief スコープを求める
 * 
 * @return
 *   スコープを読み取れた場合は AST::Scope*
 *   なければエラー
 */
AST::Scope* Parser::expect_scope() {
  if( auto ast = this->parse_scope(); ast ) {
    return ast;
  }

  Error(
    *({ auto tok = this->cur; --tok; }),
    "expected scope after this token"
  )
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
AST::Function* Parser::parse_function() {
  auto func =
    new AST::Function(
      *this->expect("fn"), *this->expect_identifier()
    ); // AST 作成

  this->expect("("); // 引数リストの開きカッコ

  // 閉じかっこがなければ、引数を読み取っていく
  if( !this->eat(")") ) {
    do {
      auto arg_name_token =
        this->expect_identifier(); // 引数名のトークン
      
      this->expect(":"); // コロン

      // 型
      func->append_argument(
        *arg_name_token, this->expect_typename());
    } while( this->eat(",") ); // カンマがあれば続ける
    
    this->expect(")"); // 閉じかっこ
  }

  this->expect("->"); // 型指定トークン

  func->result_type = this->expect_typename(); // 戻り値の型

  func->code = this->expect_scope(); // 処理 (スコープ)

  return func;
}

