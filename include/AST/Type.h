#pragma once

namespace AST {

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

}  // namespace AST