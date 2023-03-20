#pragma once

namespace AST {

struct Impl : ListBase {
  ASTVector impls;

  bool is_empty() const override
  {
    return this->impls.empty();
  }

  Base*& append(Base* x) override
  {
    return this->impls.emplace_back(x);
  }

  Impl(Token const& token)
      : ListBase(AST_Impl, token)
  {
  }

  ~Impl()
  {
  }
};

}  // namespace AST