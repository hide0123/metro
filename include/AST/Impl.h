#pragma once

namespace AST {

struct Impl : Scope {
  Typeable* type;

  std::string_view name;

  Impl(Token const& token, Typeable* type)
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