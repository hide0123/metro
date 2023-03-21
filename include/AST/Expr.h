#pragma once

namespace AST {

struct ConstKeyword : Base {
  ConstKeyword(ASTKind kind, Token const& token);
};

struct Value : Base {
  Object* object;

  std::string to_string() const;

  Value(Token const& tok);
};

struct Variable : Base {
  size_t step;
  size_t index;
  std::string_view name;

  Variable(Token const& tok);
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

  CallFunc(Token const& name);
  ~CallFunc();
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

  Item& append(Base* k, Token const& t, Base* v)
  {
    return this->elements.emplace_back(t, k, v);
  }

  Dict(Token const& token);
  ~Dict();
};

struct Cast : Base {
  Type* cast_to;
  Base* expr;

  Cast(Token const& token);
  ~Cast();
};

struct UnaryOp : Base {
  Base* expr;

  UnaryOp(ASTKind kind, Token const& token, Base* expr);

  ~UnaryOp();
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

  Vector(Token const& token);
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

  IndexRef(Token const& t);
  ~IndexRef();
};

struct Range : Base {
  Base* begin;
  Base* end;

  Range(Token const& token);
  ~Range();
};

struct Assign : Base {
  Base* dest;
  Base* expr;

  Assign(Token const& assign_op);
  ~Assign();
};

using Expr = ExprBase<ExprKind, AST_Expr>;
using Compare = ExprBase<CmpKind, AST_Compare>;

}  // namespace AST
