#include "AST.h"

namespace AST {

Base::Base(ASTKind kind, Token const& token)
    : kind(kind),
      token(token),
      end_token(nullptr)
{
}

Base::~Base()
{
}

ListBase::ASTVector::~ASTVector()
{
  for (auto&& x : *this)
    delete x;
}

ListBase::ListBase(ASTKind kind, Token const& token)
    : Base(kind, token)
{
}

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

TypeConstructor::TypeConstructor(Type* type)
    : Dict(type->token),
      type(type)
{
  this->kind = AST_TypeConstructor;
}

TypeConstructor ::~TypeConstructor()
{
}

}  // namespace AST