#pragma once

namespace AST {

//
// 変数定義
struct VariableDeclaration : Base {
  std::string_view name;
  Type* type;
  Base* init;

  size_t index;

  bool is_shadowing;
  bool is_const;

  bool ignore_initializer;

  VariableDeclaration(Token const& token);
  ~VariableDeclaration();
};

struct Return : Base {
  AST::Base* expr;

  Return(Token const& token);
  ~Return();
};

struct LoopController : Base {
  LoopController(Token const& token, ASTKind kind);
};

struct Scope : ListBase {
  ASTVector list;
  bool return_last_expr;
  bool of_function;

  bool is_empty() const override
  {
    return this->list.empty();
  }

  Base*& append(Base* item) override
  {
    return this->list.emplace_back(item);
  }

  Scope(Token const& token);
  ~Scope();
};

struct If : Base {
  Base* condition;
  Base* if_true;
  Base* if_false;

  If(Token const& token);

  ~If();
};

struct Case : Base {
  Base* cond;
  Scope* scope;

  Case(Token const& token);
  ~Case();
};

struct Switch : Base {
  Base* expr;
  std::vector<Case*> cases;

  Case*& append(Case* c)
  {
    return this->cases.emplace_back(c);
  }

  Switch(Token const& token);
  ~Switch();
};

struct For : Base {
  Base* iter;
  Base* iterable;
  Base* code;

  For(Token const& tok);
  ~For();
};

struct While : Base {
  Base* cond;
  Scope* code;

  While(Token const& tok);
  ~While();
};

struct DoWhile : Base {
  Scope* code;
  Base* cond;

  DoWhile(Token const& tok);
  ~DoWhile();
};

struct Loop : Base {
  Base* code;

  Loop(Base* code);
  ~Loop();
};

}  // namespace AST