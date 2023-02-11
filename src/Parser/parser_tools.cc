#include "metro.h"

/**
 * @brief Construct a new Parser:: Parser object
 * 
 * @param token_list 
 */
Parser::Parser(std::list<Token> const& token_list)
  : token_list(token_list),
    cur(token_list.begin()),
    ate(token_list.begin())
{
}

/**
 * @brief Destroy the Parser:: Parser object
 * 
 */
Parser::~Parser() {

}

/**
 * @brief 
 * 
 * @return AST::Base* 
 */
AST::Base* Parser::parse() {
  auto root_scope = new AST::Scope(*this->cur);

  while( this->check() ) {
    root_scope->append(this->top());

    if( this->ate->str == "}" ) {
      continue;
    }
    else if( this->check() ) {
      this->expect_semi();
    }
  }

  return root_scope;
}

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

/**
 * @brief
 * 
 * @return AST::Base* 
 */
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

AST::Base* Parser::top() {
  if( this->cur->str == "fn" ) {
    return this->parse_function();
  }

  return this->expr();
}

//
// Parser::parse_scope()
//
// スコープをパースする
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

//
// Parser::parse_function()
//
// 関数をパースする
// "fn" がある位置で呼び出すこと
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

AST::Type* Parser::parse_typename() {
  auto ast =
    new AST::Type(*this->expect_identifier());

  // todo: 修飾子を読み取る

  return ast;
}

AST::Type* Parser::expect_typename() {
  if( auto ast = this->parse_typename(); ast )
    return ast;

  Error(*({ auto tok = this->cur; --tok; }),
    "expected typename after this token"
  )
    .emit()
    .exit();
}

bool Parser::check() {
  return this->cur->kind != TOK_End;
}

void Parser::next() {
  this->cur++;
}

bool Parser::eat(char const* s) {
  if( this->cur->str == s ) {
    this->ate = this->cur++;
    return true;
  }

  return false;
}

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

bool Parser::eat_semi() {
  return this->eat(";");
}

Parser::token_iter Parser::expect_semi() {
  return this->expect(";");
}

Parser::token_iter Parser::expect_identifier() {
  if( this->cur->kind != TOK_Ident ) {
    Error(*this->cur, "expected identifier")
      .emit()
      .exit();
  }

  this->ate = this->cur++;
  return this->ate;
}

AST::Return* Parser::new_return_stmt(AST::Base* ast) {
  auto ret = new AST::Return(ast->token);

  ret->expr = ast;

  return ret;
}