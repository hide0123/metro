#pragma once

namespace AST {

struct Enum : Typeable {
  struct Enumerator {
    Token const& token;
    std::string_view name;
    Type* value_type;

    Enumerator(Token const& token, Type* value_type)
      : token(token),
        name(token.str),
        value_type(value_type)
    {
    }
  };

  std::vector<Enumerator> enumerators;

  Enum(Token const& token, Token const& name_token)
    : Typeable(AST_Enum, token)
  {
    this->name = name_token.str;
  }
};

}  // namespace AST