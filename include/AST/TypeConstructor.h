// Call a constructor of type

// Syntax:
//   ident "{" ident ":" expr ("," ident ":" expr)* "}"

#pragma once

namespace AST {

struct TypeConstructor : Base {
  struct Element {
    std::string_view name;
    Base* expr;

    Element(std::string_view name, Base* expr)
        : name(name),
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