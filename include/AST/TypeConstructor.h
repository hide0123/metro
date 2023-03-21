// Call a constructor of type

// Syntax:
//   ident "{" ident ":" expr ("," ident ":" expr)* "}"

#pragma once

namespace AST {

struct TypeConstructor : Base {
  struct Element {
    Token const& token;  // name
    std::string_view name;
    Base* expr;

    Element(Token const& token, Base* expr)
        : token(token),
          name(token.str),
          expr(expr)
    {
    }
  };

  std::string_view name;
  std::vector<Element> elements;

  TypeInfo type;

  TypeConstructor(Token const& token)
      : Base(AST_TypeConstructor, token),
        name(token.str)
  {
  }

  ~TypeConstructor()
  {
    for (auto&& x : this->elements)
      delete x.expr;
  }
};

}  // namespace AST