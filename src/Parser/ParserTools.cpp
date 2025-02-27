#include "Utils.h"
#include "Debug.h"

#include "AST/AST.h"
#include "Parser.h"
#include "Error.h"

AST::Base* Parser::set_last_token(AST::Base* ast)
{
  ast->end_token = this->cur;
  ast->end_token--;

  return ast;
}

AST::Variable* Parser::new_variable()
{
  return (AST::Variable*)this->set_last_token(
    new AST::Variable(*this->expect_identifier()));
}

/**
 * @brief スコープをパースする
 *
 * @note 最後にセミコロンがない場合、最後の要素を return
 * 文にする
 *
 * @return AST::Scope*
 */
AST::Scope* Parser::parse_scope(Parser::TokenIterator tok, AST::Base* first)
{
  if (!first && !this->eat("{")) {
    Error(*--this->cur, "expected scope after this token").emit().exit();
  }

  auto ast = new AST::Scope(first ? *tok : *this->ate);

  if (first) {
    ast->append(first);

    if (!this->eat("}")) {
      if (!first->is_ended_with_scope())
        this->expect_semi();
    }
    else {
      ast->return_last_expr = true;
      return ast;
    }
  }

  while (!this->eat("}")) {
    AST::Base* x = nullptr;

    x = ast->append(this->expr());

    if ((ast->return_last_expr = !this->eat_semi())) {
      if (!(ast->return_last_expr = this->cur->str == "}") &&
          !x->is_ended_with_scope())
        this->expect_semi();
    }
  }

  return (AST::Scope*)this->set_last_token(ast);
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
  auto func = new AST::Function(*this->expect("fn"),
                                *this->expect_identifier());  // AST 作成

  func->args_brace_begin = &*this->expect("(");  // 引数リストの開きカッコ

  // 閉じかっこがなければ、引数を読み取っていく
  if (!this->eat(")")) {
    std::map<std::string_view, bool> namemap;

    do {
      if (!(namemap[this->cur->str] ^= 1)) {
        Error(ERR_MultipleDefined, *this->cur,
              "duplication argument name '" + std::string(this->cur->str) + "'")
          .emit();
      }

      if (this->eat("self")) {
        if (!this->in_impl) {
          Error(ERR_InvalidSyntax, *this->ate, "used 'self' without impl-block")
            .emit()
            .exit();
        }

        func->have_self = true;
      }
      else {
        func->append_argument(this->expect_identifier()->str,
                              *this->expect(":"), this->expect_typename());
      }
    } while (this->eat(","));  // カンマがあれば続ける

    func->args_brace_end = &*this->expect(")");  // 閉じかっこ
  }
  else {
    func->args_brace_end = &*this->ate;
  }

  // 戻り値の型
  if (this->eat("->")) {
    func->result_type = this->expect_typename();
  }

  func->code = this->expect_scope();  // 処理 (スコープ)

  this->set_last_token(func);

  return func;
}

AST::Enum* Parser::parse_enum()
{
  auto ast = new AST::Enum(*this->expect("enum"), *this->expect_identifier());

  this->expect("{");

  if (this->eat("}")) {
    Error(ERR_EmptyEnum, ast->token, "empty enum is not valid").emit().exit();
  }

  do {
    auto& enumerator = ast->add_enumerator(*this->expect_identifier(), nullptr);

    if (this->eat("(")) {
      enumerator.value_type = this->expect_typename();
      this->expect(")");
    }
  } while (this->eat(","));

  this->expect("}");

  return ast;
}

AST::Struct* Parser::parse_struct()
{
  auto ast = new AST::Struct(*this->expect("struct"));

  ast->name = this->expect_identifier()->str;

  this->expect("{");

  if (this->eat("}")) {
    Error(ERR_EmptyStruct, *this->ate, "empty struct is not valid")
      .emit()
      .exit();
  }

  do {
    auto& member = ast->append(*this->expect_identifier(), nullptr);

    this->expect(":");

    member.type = this->expect_typename();
  } while (this->eat(","));

  ast->end_token = this->expect("}");

  return ast;
}

AST::Impl* Parser::parse_impl()
{
  this->in_impl = true;

  auto ast = new AST::Impl(*this->expect("impl"), this->expect_typename());

  this->expect("{");

  while (!this->eat("}")) {
    auto x = top();

    switch (x->kind) {
      case AST_Function:
        ast->append((AST::Function*)x);
        break;

      case AST_Impl:
        Error(ERR_InvalidSyntax, x->token, "impl-block cannot be nested")
          .emit()
          .exit();

      default:
        Error(ERR_InvalidSyntax, x->token, "impl-block must have functions")
          .emit()
          .exit();
    }
  }

  this->in_impl = false;

  return (AST::Impl*)this->set_last_token(ast);
}

/**
 * @brief 型名をパースする
 *
 * @return AST::Type*
 */
AST::Type* Parser::expect_typename()
{
  static int depth = 0;
  static bool pass_rbrace = 0;

  auto ast = new AST::Type(*this->expect_identifier());

  if (this->eat("<")) {
    depth++;

    do {
      ast->parameters.emplace_back(this->expect_typename());
    } while (this->eat(","));

    if (pass_rbrace) {
      pass_rbrace = false;
    }
    else if (depth >= 2 && this->eat(">>")) {
      depth--;
      pass_rbrace = true;
    }
    else {
      this->expect(">");
    }

    depth--;
  }

  if (this->eat("const")) {
    ast->is_const = true;
  }

  this->set_last_token(ast);

  return ast;
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
 * @return 進める前のトークン
 *
 */
Parser::TokenIterator Parser::next()
{
  return this->cur++;
}

/**
 * @brief トークン消費
 *
 * @param s: 文字列
 * @return s を食べたら true, そうでなければ false
 */
bool Parser::eat(char const* s)
{
  if (this->found(s)) {
    this->ate = this->cur++;
    return true;
  }

  return false;
}

bool Parser::found(char const* s)
{
  return this->cur->str == s;
}

/**
 * @brief 指定したトークンを求める
 *
 * @param s: 文字列
 * @return
 *  食べたらそのトークンへのイテレータ (Parser::TokenIterator)
 *  を返し、無ければエラーで終了
 */
Parser::TokenIterator Parser::expect(char const* s)
{
  if (!this->eat(s)) {
    Error(*(--this->cur), "expected '" + std::string(s) + "' after this token")
      .emit()
      .exit();
  }

  return this->ate;
}

Parser::TokenIterator Parser::get(size_t offs)
{
  auto it = this->cur;

  while (offs != 0) {
    it++;
    offs--;
  }

  return it;
}

// セミコロン消費
bool Parser::eat_semi()
{
  return this->eat(";");
}

// セミコロン求める
Parser::TokenIterator Parser::expect_semi()
{
  return this->expect(";");
}

// 識別子を求める
Parser::TokenIterator Parser::expect_identifier()
{
  if (this->cur->kind != TOK_Ident) {
    throw Error(*(--this->cur), "expected identifier after this token");
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