#pragma once

namespace AST {

//
// 変数定義
struct VariableDeclaration : Base {
  std::string_view name;
  Type* type;
  Base* init;

  VariableDeclaration(Token const& token)
      : Base(AST_Let, token),
        type(nullptr),
        init(nullptr)
  {
  }

  ~VariableDeclaration()
  {
    if (this->type)
      delete this->type;

    if (this->init)
      delete this->init;
  }
};

struct Return : Base {
  AST::Base* expr;

  Return(Token const& token)
      : Base(AST_Return, token),
        expr(nullptr)
  {
  }

  ~Return()
  {
    if (this->expr)
      delete this->expr;
  }
};

struct LoopController : Base {
  LoopController(Token const& token, ASTKind kind)
      : Base(kind, token)
  {
  }
};

struct Scope : ListBase {
  ASTVector list;
  bool return_last_expr;

  Base*& append(Base* item) override
  {
    return this->list.emplace_back(item);
  }

  Scope(Token const& token)
      : ListBase(AST_Scope, token),
        return_last_expr(false)
  {
  }

  ~Scope()
  {
  }
};

struct If : Base {
  Base* condition;
  Base* if_true;
  Base* if_false;

  If(Token const& token)
      : Base(AST_If, token),
        condition(nullptr),
        if_true(nullptr),
        if_false(nullptr)
  {
  }

  ~If()
  {
    delete this->condition;
    delete this->if_true;

    if (this->if_false)
      delete this->if_false;
  }
};

struct Case : Base {
  Base* cond;
  Scope* scope;

  Case(Token const& token)
      : Base(AST_Case, token),
        cond(nullptr),
        scope(nullptr)
  {
  }

  ~Case()
  {
    delete this->cond;
    delete this->scope;
  }
};

struct Switch : Base {
  Base* expr;
  std::vector<Case*> cases;

  Case*& append(Case* c)
  {
    return this->cases.emplace_back(c);
  }

  Switch(Token const& token)
      : Base(AST_Switch, token),
        expr(nullptr)
  {
  }

  ~Switch()
  {
    delete this->expr;

    for (auto&& c : this->cases)
      delete c;
  }
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

  ~For()
  {
    delete this->iter;
    delete this->iterable;
    delete this->code;
  }
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

  ~While()
  {
    delete this->cond;
    delete this->code;
  }
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

  ~DoWhile()
  {
    delete this->code;
    delete this->cond;
  }
};

struct Loop : Base {
  Base* code;

  Loop(Base* code)
      : Base(AST_Loop, code->token),
        code(code)
  {
  }

  ~Loop()
  {
    delete this->code;
  }
};

}  // namespace AST