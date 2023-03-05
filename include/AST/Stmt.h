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

struct Function : Base {
  struct Argument {
    Token const& name;
    AST::Type* type;

    explicit Argument(Token const& name, AST::Type* type)
        : name(name),
          type(type)
    {
    }

    ~Argument()
    {
      delete this->type;
    }
  };

  Token const& name;  // 名前
  std::vector<Argument> args;  // 引数

  Type* result_type;  // 戻り値の型
  Scope* code;  // 処理

  size_t var_count = 0;

  /**
   * @brief 引数を追加する
   *
   * @param name
   * @param type
   * @return Argument&
   */
  Argument& append_argument(Token const& name, AST::Type* type)
  {
    return this->args.emplace_back(name, type);
  }

  /**
   * @brief Construct a new Function object
   *
   * @param token
   * @param name
   */
  explicit Function(Token const& token, Token const& name)
      : Base(AST_Function, token),
        name(name),
        result_type(nullptr),
        code(nullptr)
  {
  }

  ~Function();
};

}  // namespace AST