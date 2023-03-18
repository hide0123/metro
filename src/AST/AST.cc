#include "AST.h"

namespace AST {

template <class Kind, ASTKind _self>
ExprBase<Kind, _self>::Element::Element(Kind kind,
                                        Token const& op,
                                        Base* ast)
    : kind(kind),
      op(op),
      ast(ast)
{
}

template <class Kind, ASTKind _self>
ExprBase<Kind, _self>::Element::~Element()
{
  delete this->ast;
}

template <class Kind, ASTKind _self>
std::string ExprBase<Kind, _self>::to_string() const
{
  auto s = this->first->to_string();

  for (auto&& elem : this->elements) {
    s += " " + std::string(elem.op.str) + " " +
         elem.ast->to_string();
  }

  return s;
}

template <class Kind, ASTKind _self>
typename ExprBase<Kind, _self>::Element&
ExprBase<Kind, _self>::append(Kind kind, Token const& op,
                              Base* ast)
{
  return this->elements.emplace_back(kind, op, ast);
}

template <class Kind, ASTKind _self>
ExprBase<Kind, _self>* ExprBase<Kind, _self>::create(Base*& ast)
{
  if (ast->kind != _self)
    ast = new ExprBase<Kind, _self>(ast);

  return (ExprBase*)ast;
}

template <class Kind, ASTKind _self>
ExprBase<Kind, _self>::ExprBase(Base* first)
    : Base(_self, first->token),
      first(first)
{
}

template <class Kind, ASTKind _self>
ExprBase<Kind, _self>::~ExprBase()
{
  delete this->first;
}

template struct ExprBase<ExprKind, AST_Expr>;
template struct ExprBase<CmpKind, AST_Compare>;

Argument::Argument(std::string_view const& name,
                   Token const& colon, AST::Type* type)
    : Base(AST_Argument, colon),
      name(name),
      type(type)
{
}

Argument::~Argument()
{
  delete this->type;
}

Function::Function(Token const& token, Token const& name)
    : Base(AST_Function, token),
      name(name),
      result_type(nullptr)
{
}

Function::~Function()
{
  for (auto&& arg : this->args) {
    delete arg;
  }

  if (this->result_type)
    delete this->result_type;

  delete this->code;
}

Type::Type(Token const& token)
    : Base(AST_Type, token),
      is_const(false)
{
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

Case::Case(Token const& token)
    : Base(AST_Case, token),
      cond(nullptr),
      scope(nullptr)
{
}

Case::~Case()
{
  delete this->cond;
  delete this->scope;
}

Switch::Switch(Token const& token)
    : Base(AST_Switch, token),
      expr(nullptr)
{
}

Switch::~Switch()
{
  delete this->expr;

  for(auto&&x:this->cases)
    delete x;
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

}  // namespace AST
