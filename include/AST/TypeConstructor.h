// Call a constructor of type

// Syntax:
//   ident "{" ident ":" expr ("," ident ":" expr)* "}"

#pragma once

namespace AST {

struct TypeConstructor : Dict {
  Type* type;
  TypeInfo typeinfo;

  explicit TypeConstructor(Type* type)
      : Dict(type->token),
        type(type)
  {
    this->kind = AST_TypeConstructor;
  }

  ~TypeConstructor()
  {
  }
};

}  // namespace AST