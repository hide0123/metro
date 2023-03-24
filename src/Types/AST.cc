#include "AST.h"

namespace AST {

Base::Base(ASTKind kind, Token const& token)
  : kind(kind),
    token(token),
    end_token(nullptr),
    is_left(false)
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

Typeable::Typeable(ASTKind kind, Token const& token)
  : Base(kind, token)
{
}

Typeable::~Typeable()
{
}

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

UnaryOp::UnaryOp(ASTKind kind, Token const& token, Base* expr)
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

IndexRef::IndexRef(Token const& t, Base* expr)
  : Base(AST_IndexRef, t),
    expr(expr),
    is_enum(false),
    enum_ast(nullptr)
{
}

IndexRef::~IndexRef()
{
  delete this->expr;

  for (auto&& sub : this->indexes) {
    delete sub.ast;
  }
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

VariableDeclaration::VariableDeclaration(Token const& token)
  : Base(AST_Let, token),
    type(nullptr),
    init(nullptr)
{
}

VariableDeclaration ::~VariableDeclaration()
{
  if (this->type)
    delete this->type;

  if (this->init)
    delete this->init;
}

Return::Return(Token const& token)
  : Base(AST_Return, token),
    expr(nullptr)
{
}

Return::~Return()
{
  if (this->expr)
    delete this->expr;
}

LoopController ::LoopController(Token const& token, ASTKind kind)
  : Base(kind, token)
{
}

Scope::Scope(Token const& token)
  : ListBase(AST_Scope, token),
    return_last_expr(false)
{
}

Scope ::~Scope()
{
}
If::If(Token const& token)
  : Base(AST_If, token),
    condition(nullptr),
    if_true(nullptr),
    if_false(nullptr)
{
}
If ::~If()
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

  for (auto&& c : this->cases)
    delete c;
}

For::For(Token const& tok)
  : Base(AST_For, tok),
    iter(nullptr),
    iterable(nullptr),
    code(nullptr)
{
}

For::~For()
{
  delete this->iter;
  delete this->iterable;
  delete this->code;
}

While::While(Token const& tok)
  : Base(AST_While, tok),
    cond(nullptr),
    code(nullptr)
{
}

While::~While()
{
  delete this->cond;
  delete this->code;
}

DoWhile::DoWhile(Token const& tok)
  : Base(AST_DoWhile, tok),
    code(nullptr),
    cond(nullptr)
{
}

DoWhile::~DoWhile()
{
  delete this->code;
  delete this->cond;
}

Loop::Loop(Base* code)
  : Base(AST_Loop, code->token),
    code(code)
{
}

Loop::~Loop()
{
  delete this->code;
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
  delete this->type;
}

}  // namespace AST