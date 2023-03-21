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

  ~UnaryOp()
  {
    delete this->expr;
  }
};

struct Cast : Base {
  Type* cast_to;
  Base* expr;

  Cast(Token const& token)
      : Base(AST_Cast, token)
  {
  }

  ~Cast()
  {
    delete this->cast_to;
    delete this->expr;
  }
};

struct Value : Base {
  std::string to_string() const;

  Value(Token const& tok)
      : Base(AST_Value, tok)
  {
  }
};

struct Vector : ListBase {
  ASTVector elements;

  bool is_empty() const override
  {
    return this->elements.empty();
  }

  Base*& append(Base* ast) override
  {
    return this->elements.emplace_back(ast);
  }

  Vector(Token const& token)
      : ListBase(AST_Vector, token)
  {
  }
};

struct Dict : Base {
  struct Item {
    Token const& colon;
    Base* key;
    Base* value;

    Item(Token const& colon, Base* k, Base* v)
        : colon(colon),
          key(k),
          value(v)
    {
    }

    ~Item()
    {
    }
  };

  std::vector<Item> elements;

  Type* key_type;
  Type* value_type;

  Item& append(Base* k, Token const& t, Base* v)
  {
    return this->elements.emplace_back(t, k, v);
  }

  Dict(Token const& token)
      : Base(AST_Dict, token),
        key_type(nullptr),
        value_type(nullptr)
  {
  }

  ~Dict()
  {
    if (this->key_type)
      delete this->key_type;

    if (this->value_type)
      delete this->value_type;

    for (auto&& item : this->elements) {
      delete item.key;
      delete item.value;
    }
  }
};

struct Variable : Base {
  size_t step;
  size_t index;

  Variable(Token const& tok)
      : Base(AST_Variable, tok),
        step(0),
        index(0)
  {
  }
};

struct IndexRef : ListBase {
  Base* expr;
  ASTVector indexes;

  bool is_empty() const override
  {
    return this->indexes.empty();
  }

  Base*& append(Base* x) override
  {
    return this->indexes.emplace_back(x);
  }

  IndexRef(Token const& t)
      : ListBase(AST_IndexRef, t),
        expr(nullptr)
  {
  }

  ~IndexRef()
  {
    delete this->expr;
  }
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

  ~Range()
  {
    delete begin;
    delete end;
  }
};

struct CallFunc : ListBase {
  std::string_view name;
  ASTVector args;

  bool is_builtin;
  BuiltinFunc const* builtin_func;
  Function* callee;

  std::string to_string() const override;

  bool is_empty() const override
  {
    return this->args.empty();
  }

  Base*& append(Base* ast) override
  {
    return this->args.emplace_back(ast);
  }

  CallFunc(Token const& name)
      : ListBase(AST_CallFunc, name),
        name(name.str),
        is_builtin(false),
        builtin_func(nullptr),
        callee(nullptr)
  {
  }

  ~CallFunc()
  {
  }
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

  ~Assign()
  {
    delete dest;
    delete expr;
  }
};

using Expr = ExprBase<ExprKind, AST_Expr>;
using Compare = ExprBase<CmpKind, AST_Compare>;

}  // namespace AST
