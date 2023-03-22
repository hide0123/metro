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

struct TypeConstructor : Dict {
  Type* type;
  TypeInfo typeinfo;

  explicit TypeConstructor(Type* type);
  ~TypeConstructor();
};

}  // namespace AST