#pragma once

namespace AST {

struct Impl : ListBase {
  Token const& name_token;
  std::string_view name;
  ASTVector impls;

  bool is_empty() const override
  {
    return this->impls.empty();
  }

  Base*& append(Base* x) override
  {
    return this->impls.emplace_back(x);
  }

  Impl(Token const& token, Token const& name_token)
    : ListBase(AST_Impl, token),
      name_token(name_token),
      name(name_token.str)
  {
  }

  ~Impl()
  {
  }
};

}  // namespace AST