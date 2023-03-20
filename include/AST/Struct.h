#pragma once

namespace AST {

struct Struct : Base {
  struct Item {
    Token const& token;
    Type* type;

    std::string_view name;

    Item(Token const& token, Type* type)
        : token(token),
          type(type),
          name(token.str)
    {
    }
  };

  std::string_view name;

  std::vector<Item> items;

  Item& append(Token const& token, Type* type)
  {
    return this->items.emplace_back(token, type);
  }

  Struct(Token const& token)
      : Base(AST_Struct, token)
  {
  }

  ~Struct()
  {
    for (auto&& item : this->items)
      delete item.type;
  }
};

}  // namespace AST