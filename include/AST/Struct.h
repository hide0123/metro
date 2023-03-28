#pragma once

namespace AST {

struct Struct : Typeable {
  struct Member {
    Token const& token;
    Type* type;

    std::string_view name;

    Member(Token const& token, Type* type)
      : token(token),
        type(type),
        name(token.str)
    {
    }
  };

  std::vector<Member> members;

  Member& append(Token const& token, Type* type)
  {
    return this->members.emplace_back(token, type);
  }

  Struct(Token const& token)
    : Typeable(AST_Struct, token)
  {
  }

  ~Struct()
  {
    for (auto&& m : this->members)
      delete m.type;
  }
};

}  // namespace AST