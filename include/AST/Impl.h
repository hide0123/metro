#pragma once

namespace AST {

struct Impl : Scope {
  Token const& name_token;
  std::string_view name;

  Impl(Token const& token, Token const& name_token)
    : Scope(token),
      name_token(name_token),
      name(name_token.str)
  {
    this->kind = AST_Impl;
  }

  ~Impl()
  {
  }
};

}  // namespace AST