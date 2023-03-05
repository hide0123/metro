#include "AST.h"

namespace AST {

std::string Base::to_string() const
{
  return std::string(this->token.str);
}

std::string Value::to_string() const
{
  if (this->token.kind == TOK_String)
    return '"' + std::string(this->token.str) + '"';

  return std::string(this->token.str);
}

std::string CallFunc::to_string() const
{
  auto ret = std::string(this->name) + "(";

  for (auto&& arg : this->args) {
    ret += arg->to_string();
    if (arg != *this->args.rbegin())
      ret += ",";
  }

  return ret + ")";
}

}  // namespace AST