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

  TypeConstructor(Token const& token)
      : Base(AST_TypeConstructor, token),
        name(token.str),
        expr(nullptr)
  {
  }

  ~TypeConstructor()
  {
    for (auto&& x : this->elements)
      delete x.expr;
  }
};

}  // namespace AST