// Call a constructor of type

// Syntax:
//   ident "{" ident ":" expr ("," ident ":" expr)* "}"

#pragma once

namespace AST {

struct TypeConstructor : Dict {
  Type* type;
  TypeInfo typeinfo;

  explicit TypeConstructor(Type* type);
  ~TypeConstructor();
};

}  // namespace AST