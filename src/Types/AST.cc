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

Type::~Type()
{
  for (auto&& p : this->parameters)
    delete p;
}

UnaryOp::~UnaryOp()
{
  delete this->expr;
}

Cast::~Cast()
{
  delete this->cast_to;
  delete this->expr;
}

Vector::~Vector()
{
  for (auto&& e : this->elements)
    delete e;
}

Dict::Item::Item(Token const& colon, Base* k, Base* v)
    : colon(colon),
      key(k),
      value(v)
{
}

Dict::Item::~Item()
{
  delete this->key;
  delete this->value;
}

Dict::~Dict()
{
  if (this->key_type)
    delete this->key_type;

  if (this->value_type)
    delete this->value_type;
}

IndexRef::~IndexRef()
{
  delete this->expr;

  for (auto&& i : this->indexes)
    delete i;
}

Range::~Range()
{
  delete this->begin;
  delete this->end;
}

CallFunc::~CallFunc()
{
  for (auto&& arg : this->args)
    delete arg;
}

Assign::~Assign()
{
  if (this->dest)
    delete this->dest;

  if (this->expr)
    delete this->expr;
}

VariableDeclaration::~VariableDeclaration()
{
  if (this->type)
    delete this->type;

  if (this->init)
    delete this->init;
}

Return::~Return()
{
  if (this->expr)
    delete this->expr;
}

Scope::~Scope()
{
  for (auto&& x : this->list)
    delete x;
}

If::~If()
{
  delete this->condition;
  delete this->if_true;

  if (this->if_false)
    delete this->if_false;
}

For::~For()
{
  delete this->iter;
  delete this->iterable;
  delete this->code;
}

While::~While()
{
  delete this->cond;
  delete this->code;
}

DoWhile::~DoWhile()
{
  delete this->code;
  delete this->cond;
}

Loop::~Loop()
{
  delete this->code;
}

Function::~Function()
{
  if (this->result_type)
    delete this->result_type;

  delete this->code;
}

}  // namespace AST
