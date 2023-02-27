#include "common.h"

#include "AST.h"
#include "Parser.h"

#include "Error.h"

using EXKind = AST::Expr::ExprKind;

Parser::Parser(std::list<Token> const& token_list)
  : cur(token_list.begin()),
    ate(token_list.begin())
{
}

Parser::~Parser() {

}

AST::Scope* Parser::parse() {
  auto root_scope = new AST::Scope(*this->cur);

  while( this->check() ) {
    root_scope->append(this->top());
  }

  return root_scope;
}


//
// primary
AST::Base* Parser::primary() {
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
      return new AST::Variable(*ident);
    }

    default:
      Error(*this->cur, "invalid syntax")
        .emit()
        .exit();
  }
  
  return this->stmt();
}

AST::Base* Parser::mul() {
  auto x = this->primary();

  while( this->check() ) {
    if( this->eat("*") )
      AST::Expr::create(x)->append(
        EXKind::EX_Mul, *this->ate, this->primary());
    else if( this->eat("/") )
      AST::Expr::create(x)->append(
        EXKind::EX_Div, *this->ate, this->primary());
    else if( this->eat("%") )
      AST::Expr::create(x)->append(
        EXKind::EX_Mod, *this->ate, this->primary());
    else
      break;
  }

  return x;
}

AST::Base* Parser::add() {
  auto x = this->mul();

  while( this->check() ) {
    if( this->eat("+") )
      AST::Expr::create(x)->append(
        EXKind::EX_Add, *this->ate, this->mul());
    else if( this->eat("-") )
      AST::Expr::create(x)->append(
        EXKind::EX_Sub, *this->ate, this->mul());
    else
      break;
  }

  return x;
}

AST::Base* Parser::shift() {
  auto x = this->add();

  while( this->check() ) {
    if( this->eat("<<") )
      AST::Expr::create(x)->append(
        EXKind::EX_LShift, *this->ate, this->add());
    else if( this->eat(">>") )
      AST::Expr::create(x)->append(
        EXKind::EX_RShift, *this->ate, this->add());
    else
      break;
  }

  return x;
}

AST::Base* Parser::compare() {
  auto x = this->shift();

  while( this->check() ) {
    if( this->eat("==") )
      AST::Compare::create(x)->append(
        AST::Compare::CMP_Equal, *this->ate, this->shift());
    else if( this->eat("!=") )
      AST::Compare::create(x)->append(
        AST::Compare::CMP_Equal, *this->ate, this->shift());
    else if( this->eat(">=") )
      AST::Compare::create(x)->append(
        AST::Compare::CMP_LeftBigOrEqual, *this->ate, this->shift());
    else if( this->eat("<=") )
      AST::Compare::create(x)->append(
        AST::Compare::CMP_RightBigOrEqual, *this->ate, this->shift());
    else if( this->eat(">") )
      AST::Compare::create(x)->append(
        AST::Compare::CMP_LeftBigger, *this->ate, this->shift());
    else if( this->eat("<") )
      AST::Compare::create(x)->append(
        AST::Compare::CMP_RightBigger, *this->ate, this->shift());
    else
      break;
  }

  return x;
}

AST::Base* Parser::bit_op() {
  auto x = this->compare();

  while( this->check() ) {
    if( this->eat("&") )
      AST::Expr::create(x)->append(
        EXKind::EX_BitAND, *this->ate, this->compare());
    else if( this->eat("^") )
      AST::Expr::create(x)->append(
        EXKind::EX_BitXOR, *this->ate, this->compare());
    else if( this->eat("|") )
      AST::Expr::create(x)->append(
        EXKind::EX_BitOR, *this->ate, this->compare());
    else
      break;
  }

  return x;
}

AST::Base* Parser::expr() {
  return this->bit_op();
}

/**
 * @brief ステートメント
 * 
 * @return AST::Base* 
 */
AST::Base* Parser::stmt() {
  //
  // スコープ
  if( this->cur->str == "{" )
    return this->parse_scope();

  //
  // if 文
  if( this->eat("if") ) {
    auto ast = new AST::If(*this->ate);

    ast->condition = this->expr();

    ast->if_true = this->expect_scope();

    if( this->eat("else") ) {
      ast->if_false = this->stmt();
    }

    return ast;
  }

  //
  // let 変数定義
  if( this->eat("let") ) {
    auto ast = new AST::VariableDeclaration(*this->ate);

    ast->name = this->expect_identifier()->str;

    if( this->eat(":") ) {
      ast->type = this->parse_typename();
    }

    if( this->eat("=") ) {
      ast->init = this->expr();
    }

    this->expect_semi();

    return ast;
  }

  //
  // return 文
  if( this->eat("return") ) {
    auto ast = new AST::Return(*this->ate);

    if( this->eat_semi() )
      return ast;

    ast->expr = this->expr();
    this->expect_semi();

    return ast;
  }


  auto ast = this->expr();

  this->expect_semi();

  return ast;
}


AST::Base* Parser::top() {
  if( this->cur->str == "fn" ) {
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
AST::Scope* Parser::parse_scope() {
  auto ast =
    new AST::Scope(*this->expect("{"));
  
  while( this->check() && !this->eat("}") ) {
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
    Error(*(--this->cur),
      "expected '" + std::string(s) + "' after this token")
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