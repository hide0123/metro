#include "AST.h"

namespace AST {

ConstKeyword::ConstKeyword(ASTKind kind, Token const& token)
    : Base(kind, token)
{
}

Value::Value(Token const& tok)
    : Base(AST_Value, tok),
      object(nullptr)
{
}

Variable::Variable(Token const& tok)
    : Base(AST_Variable, tok),
      step(0),
      index(0),
      name(tok.str)
{
  this->is_left = true;
}

CallFunc::CallFunc(Token const& name)
    : ListBase(AST_CallFunc, name),
      name(name.str),
      is_builtin(false),
      builtin_func(nullptr),
      callee(nullptr)
{
}

CallFunc::~CallFunc()
{
}

Dict::Item::Item(Token const& colon, Base* k, Base* v)
    : colon(colon),
      key(k),
      value(v)
{
}

Dict::Item::~Item()
{
}

Dict::Dict(Token const& token)
    : Base(AST_Dict, token),
      key_type(nullptr),
      value_type(nullptr)
{
}

Dict::~Dict()
{
  if (this->key_type)
    delete this->key_type;

  if (this->value_type)
    delete this->value_type;

  for (auto&& item : this->elements) {
    delete item.key;
    delete item.value;
  }
}

Cast::Cast(Token const& token)
    : Base(AST_Cast, token)
{
}

Cast::~Cast()
{
  delete this->cast_to;
  delete this->expr;
}

UnaryOp::UnaryOp(ASTKind kind, Token const& token,
                 Base* expr)
    : Base(kind, token),
      expr(expr)
{
}

UnaryOp::~UnaryOp()
{
  delete this->expr;
}

Vector::Vector(Token const& token)
    : ListBase(AST_Vector, token)
{
}

IndexRef::IndexRef(Token const& t)
    : ListBase(AST_IndexRef, t),
      expr(nullptr)
{
}

IndexRef::~IndexRef()
{
  delete this->expr;
}

Range::Range(Token const& token)
    : Base(AST_Range, token),
      begin(nullptr),
      end(nullptr)
{
}

Range::~Range()
{
  delete begin;
  delete end;
}

Assign::Assign(Token const& assign_op)
    : Base(AST_Assign, assign_op),
      dest(nullptr),
      expr(nullptr)
{
}

Assign::~Assign()
{
  delete dest;
  delete expr;
}

}  // namespace AST