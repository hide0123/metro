#include "AST/AST.h"
#include "Utils.h"

namespace AST {

Base::Base(ASTKind kind, Token const& token)
  : kind(kind),
    token(token),
    // end_token(nullptr),
    is_lvalue(false),
    use_default(false)
{
}

Base::~Base()
{
}

bool Base::is_empty_scope() const
{
  return this->kind == AST_Scope && ((AST::Scope*)this)->is_empty();
}

bool Base::is_empty_vector() const
{
  return this->kind == AST_Vector && ((AST::Vector*)this)->is_empty();
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
  this->is_lvalue = true;
}

CallFunc::CallFunc(Token const& name)
  : ListBase(AST_CallFunc, name),
    name(name.str),
    is_builtin(false),
    builtin_func(nullptr),
    callee(nullptr),
    is_membercall(false)
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
    enum_type(nullptr),
    enumerator_index(0),
    ignore_first(false)
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
    init(nullptr),
    is_shadowing(false),
    index(0),
    ignore_initializer(false)
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
    return_last_expr(false),
    of_function(false)
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

std::string Base::to_string(Base* _ast)
{
#define astdef(T) auto ast = (AST::T*)_ast

  switch (_ast->kind) {
    case AST_None:
    case AST_Value:
    case AST_Variable:
    case AST_True:
    case AST_False:
      break;

    case AST_CallFunc: {
      astdef(CallFunc);

      std::string ret;

      // if (ast->selftype) {
      //   ret = std::string(ast->selftype->name) + ".";
      // }

      ret += std::string(ast->name) + "(";

      for (auto&& arg : ast->args) {
        ret += to_string(arg);
        if (arg != *ast->args.rbegin())
          ret += ",";
      }

      return ret + ")";
    }

    case AST_StructConstructor: {
      astdef(StructConstructor);

      return "new " + to_string(ast->type) + "{" +
             Utils::String::join(", ", ast->init_pair_list,
                                 [](StructConstructor::Pair& pair) {
                                   return "." + std::string(pair.name) + ": " +
                                          to_string(pair.expr);
                                 }) +
             "}";
    }

    case AST_IndexRef: {
      astdef(IndexRef);

      std::string ret = to_string(ast->expr);

      for (auto&& index : ast->indexes) {
        switch (index.kind) {
          case IndexRef::Subscript::SUB_Index:
            ret += "[" + to_string(index.ast) + "]";
            break;

          case IndexRef::Subscript::SUB_Member:
          case IndexRef::Subscript::SUB_CallFunc:
            ret += "." + to_string(index.ast);
            break;
        }
      }

      return ret;
    }

    case AST_Assign: {
      astdef(Assign);

      return to_string(ast->dest) + " = " + to_string(ast->expr);
    }

    case AST_Let: {
      astdef(VariableDeclaration);

      std::string ret = "let " + std::string(ast->name);

      if (ast->type)
        ret += " : " + to_string(ast->type);

      if (ast->init)
        ret += " = " + to_string(ast->init);

      return ret;
    }

    case AST_Return: {
      astdef(Return);

      if (ast->expr)
        return "return " + to_string(ast->expr);

      return "return";
    }

    case AST_Scope: {
      astdef(Scope);

      std::string ret =
        "{ " + Utils::String::join(";\n", ast->list, [](auto& x) {
          return to_string(x);
        });

      if (!ast->return_last_expr)
        ret += ";";

      return ret + "}";
    }

    case AST_Argument: {
      astdef(Argument);

      return std::string(ast->name) + ": " + to_string(ast->type);
    }

    case AST_Function: {
      astdef(Function);

      std::string ret = "fn " + std::string(ast->name.str) + "(";

      if (ast->have_self) {
        ret += "self";

        if (!ast->args.empty())
          ret += ", ";
      }

      ret += Utils::String::join(", ", ast->args, to_string) + ")";

      if (ast->result_type)
        ret += " -> " + to_string(ast->result_type) + " ";

      return ret + to_string(ast->code);
    }

    case AST_Enum: {
      astdef(Enum);

      std::string ret = "enum " + std::string(ast->name) + " {";

      for (auto&& e : ast->enumerators) {
        ret += std::string(e.name);

        if (e.value_type)
          ret += "(" + to_string(e.value_type) + ")";

        if (&e != &*ast->enumerators.rbegin())
          ret += ", ";
      }

      return ret + "}";
    }

    case AST_Struct: {
      astdef(Struct);

      std::string ret = "struct " + std::string(ast->name) + " {";

      for (auto&& m : ast->members) {
        ret += std::string(m.name) + ": " + to_string(m.type);

        if (&m != &*ast->members.rbegin())
          ret += ", ";
      }

      return ret + "}";
    }

    case AST_Impl: {
      astdef(Impl);

      return "impl " + to_string(ast->type) + "{" +
             Utils::String::join(" ", ast->list, to_string) + "}";
    }
  }

  return std::string(_ast->token.str);
}

StructConstructor::StructConstructor(Token const& token, Type* type)
  : Base(AST_StructConstructor, token),
    type(type),
    p_struct(nullptr)

{
  this->kind = AST_StructConstructor;
}

StructConstructor ::~StructConstructor()
{
  delete this->type;

  for (auto&& pair : this->init_pair_list) {
    delete pair.expr;
  }
}

}  // namespace AST