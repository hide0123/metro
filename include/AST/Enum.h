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

  template <class... Args>
  requires std::is_constructible_v<Enumerator, Args...>
  Enumerator& add_enumerator(Args&&... args)
  {
    return this->enumerators.emplace_back(std::forward<Args>(args)...);
  }

  Enum(Token const& token, Token const& name_token)
    : Typeable(AST_Enum, token)
  {
    this->name = name_token.str;
  }
};

}  // namespace AST