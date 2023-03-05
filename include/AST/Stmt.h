#pragma once

namespace AST {

//
// 変数定義
struct VariableDeclaration : Base {
  std::string_view name;
  Type* type;
  Base* init;

  explicit VariableDeclaration(Token const& token)
      : Base(AST_Let, token),
        type(nullptr),
        init(nullptr)
  {
  }

  ~VariableDeclaration();
};

struct Return : Base {
  AST::Base* expr;

  explicit Return(Token const& token)
      : Base(AST_Return, token),
        expr(nullptr)
  {
  }

  ~Return();
};

struct LoopController : Base {
  LoopController(Token const& token, ASTKind kind)
      : Base(kind, token)
  {
  }
};

struct Scope : Base {
  std::vector<Base*> list;
  size_t var_count;

  Base*& append(Base* item)
  {
    return this->list.emplace_back(item);
  }

  explicit Scope(Token const& token)
      : Base(AST_Scope, token),
        var_count(0)
  {
  }

  ~Scope();
};

struct If : Base {
  Base* condition;
  Base* if_true;
  Base* if_false;

  explicit If(Token const& token)
      : Base(AST_If, token),
        condition(nullptr),
        if_true(nullptr),
        if_false(nullptr)
  {
  }

  ~If();
};

struct For : Base {
  Base* iter;
  Base* iterable;
  Base* code;

  For(Token const& tok)
      : Base(AST_For, tok),
        iter(nullptr),
        iterable(nullptr),
        code(nullptr)
  {
  }

  ~For();
};

struct While : Base {
  Base* cond;
  Scope* code;

  While(Token const& tok)
      : Base(AST_While, tok),
        cond(nullptr),
        code(nullptr)
  {
  }

  ~While();
};

struct DoWhile : Base {
  Scope* code;
  Base* cond;

  DoWhile(Token const& tok)
      : Base(AST_DoWhile, tok),
        code(nullptr),
        cond(nullptr)
  {
  }

  ~DoWhile();
};

struct Loop : Base {
  Base* code;

  Loop(Base* code)
      : Base(AST_Loop, code->token),
        code(code)
  {
  }

  ~Loop();
};

}  // namespace AST