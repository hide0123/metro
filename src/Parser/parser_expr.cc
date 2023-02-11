#include "metro.h"

//
// primary
AST::Base* Parser::primary() {
  //
  // スコープ
  if( this->cur->str == "{" )
    return this->parse_scope();

  //
  // トークンの種類ごとの処理
  switch( this->cur->kind ) {
    // 即値・リテラル
    case TOK_Int:
    case TOK_Float:
    case TOK_Char:
    case TOK_String:
    {
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
      if( this->eat("(") ) {
        // AST 作成
        auto callFunc = new AST::CallFunc(*ident);

        // 引数をパースする
        if( !this->eat(")") ) {
          do {
            // 式を追加
            callFunc->args.emplace_back(this->expr());
          } while( this->eat(",") ); // カンマがある限り続く

          // 閉じかっこ
          this->expect(")");
        }

        return callFunc;
      }

      // 開きかっこがなければ 変数
      return new AST::Variable(*this->cur);
    }
  }

  alert;
  print_variable("%d", this->cur->kind);

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
