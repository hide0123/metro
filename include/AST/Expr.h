#pragma once

namespace AST {

struct ConstKeyword : Base {
  ConstKeyword(ASTKind kind, Token const& token)
      : Base(kind, token)
  {
  }
};

struct UnaryOp : Base {
  Base* expr;

  UnaryOp(ASTKind kind, Token const& token, Base* expr)
      : Base(kind, token),
        expr(expr)
  {
  }

  ~UnaryOp();
};

struct Cast : Base {
  Type* cast_to;
  Base* expr;

  Cast(Token const& token)
      : Base(AST_Cast, token)
  {
  }

  ~Cast();
};

struct Value : Base {
  std::string to_string() const;

  Value(Token const& tok)
      : Base(AST_Value, tok)
  {
  }
};

struct Vector : Base {
  std::vector<Base*> elements;

  Base*& append(Base* ast)
  {
    return this->elements.emplace_back(ast);
  }

  Vector(Token const& token)
      : Base(AST_Vector, token)
  {
  }

  ~Vector();
};

struct Dict : Base {
  struct Item {
    Token const& colon;
    Base* key;
    Base* value;

    Item(Token const& colon, Base* k, Base* v);
    ~Item();
  };

  std::vector<Item> elements;

  Type* key_type;
  Type* value_type;

  Dict(Token const& token)
      : Base(AST_Dict, token),
        key_type(nullptr),
        value_type(nullptr)
  {
  }

  ~Dict();
};

struct Variable : Base {
  size_t index;

  Variable(Token const& tok)
      : Base(AST_Variable, tok),
        index(0)
  {
  }
};

struct IndexRef : Base {
  Base* expr;
  std::vector<Base*> indexes;

  IndexRef(Token const& t)
      : Base(AST_IndexRef, t),
        expr(nullptr)
  {
  }

  ~IndexRef();
};

struct Range : Base {
  Base* begin;
  Base* end;

  Range(Token const& token)
      : Base(AST_Range, token),
        begin(nullptr),
        end(nullptr)
  {
  }

  ~Range();
};

struct CallFunc : Base {
  std::string_view name;
  std::vector<Base*> args;

  bool is_builtin;
  BuiltinFunc const* builtin_func;
  Function* callee;

  std::string to_string() const;

  CallFunc(Token const& name)
      : Base(AST_CallFunc, name),
        name(name.str),
        is_builtin(false),
        builtin_func(nullptr),
        callee(nullptr)
  {
  }

  ~CallFunc();
};

struct Assign : Base {
  Base* dest;
  Base* expr;

  Assign(Token const& assign_op)
      : Base(AST_Assign, assign_op),
        dest(nullptr),
        expr(nullptr)
  {
  }

  ~Assign();
};

using Expr = ExprBase<ExprKind, AST_Expr>;
using Compare = ExprBase<CmpKind, AST_Compare>;

}  // namespace AST
