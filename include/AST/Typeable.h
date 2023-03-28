#pragma once

namespace AST {

struct Typeable : Base {
  std::string_view name;

  std::vector<Impl*> implements;

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

struct StructConstructor : Base {
  struct Pair {
    Token const& t_name;
    Token const& t_colon;

    std::string_view name;
    Base* expr;

    Pair(Token const& name, Token const& colon, Base* expr)
      : t_name(name),
        t_colon(colon),
        name(name.str),
        expr(expr)
    {
    }
  };

  Type* type;
  Struct* p_struct;
  std::vector<Pair> init_pair_list;

  explicit StructConstructor(Token const& token, Type* s);
  ~StructConstructor();
};

}  // namespace AST