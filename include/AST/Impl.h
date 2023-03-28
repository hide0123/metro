#pragma once

namespace AST {

struct Impl : Scope {
  Type* type;

  std::string_view name;

  Impl(Token const& token, Type* type)
    : Scope(token),
      type(type),
      name(type->token.str)
  {
    this->kind = AST_Impl;
  }

  ~Impl()
  {
    delete type;
  }
};

}  // namespace AST