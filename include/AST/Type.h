#pragma once

namespace AST {

struct Type : Base {
  std::vector<Type*> parameters;
  bool is_const;

  Type(Token const& token);
  ~Type();
};

}  // namespace AST