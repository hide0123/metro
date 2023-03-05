#pragma once

namespace AST {

struct Type : Base {
  std::vector<Type*> parameters;
  bool is_const;

  explicit Type(Token const& token)
      : Base(AST_Type, token),
        is_const(false)
  {
  }

  ~Type();
};

}  // namespace AST