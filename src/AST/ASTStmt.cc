#include "AST.h"

namespace AST {

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

LoopController ::LoopController(Token const& token,
                                ASTKind kind)
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

}  // namespace AST