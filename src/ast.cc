#include "metro.h"

namespace AST {

std::string Value::to_string() const {
  if( this->token.kind == TOK_String )
    return '"' + std::string(this->token.str) + '"';

  return std::string(this->token.str);
}

std::string Expr::to_string() const {
  auto s = this->first->to_string();

  for( auto&& elem : this->elements ) {
    s +=
      " " + std::string(elem.op.str) + " "
      + elem.ast->to_string();
  }

  return s;
}


} // namespace AST


