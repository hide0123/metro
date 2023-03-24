#pragma once

namespace AST {

struct Typeable : Base {
  std::string_view name;

  ~Typeable();

protected:
  Typeable(ASTKind kind, Token const& token);
};

struct Type : Typeable {
  std::vector<Type*> parameters;
  bool is_const;

  Type(Token const& token)
    : Typeable(AST_Type, token),
      is_const(false)
  {
    this->name = token.str;
  }

  ~Type()
  {
    for (auto&& p : this->parameters)
      delete p;
  }
};

struct TypeConstructor : Base {
  struct Pair {
    Token const* t_period;
    Token const* t_name;
    Token const* t_assign;

    std::string_view name;
    Base* expr;

    Pair(Token const* name, Base* expr)
      : t_period(nullptr),
        t_name(name),
        t_assign(nullptr),
        name(name->str),
        expr(expr)
    {
    }
  };

  Type* type;
  // TypeInfo typeinfo;

  std::vector<Pair> init_pair_list;

  explicit TypeConstructor(Type* type);
  ~TypeConstructor();
};

}  // namespace AST