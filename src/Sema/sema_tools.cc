#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Sema.h"

AST::Function* Sema::find_function(std::string_view name)
{
  for (auto&& item : this->root->list)
    if (item->kind == AST_Function &&
        ((AST::Function*)item)->name.str == name)
      return (AST::Function*)item;

  return nullptr;
}

BuiltinFunc const* Sema::find_builtin_func(std::string_view name)
{
  for (auto&& builtinfunc : BuiltinFunc::get_builtin_list())
    if (builtinfunc.name == name)
      return &builtinfunc;

  return nullptr;
}

Sema::ScopeEmu& Sema::get_cur_scope()
{
  return *this->scope_list.begin();
}

AST::Function* Sema::get_cur_func()
{
  return *this->function_history.begin();
}

void Sema::begin_capture(Sema::CaptureFunction cap_func)
{
  this->captures.emplace_back(cap_func);
}

void Sema::end_capture()
{
  this->captures.pop_back();
}

void Sema::begin_return_capture(
    Sema::ReturnCaptureFunction cap_func)
{
  this->return_captures.emplace_back(cap_func);
}

void Sema::end_return_capture()
{
  this->return_captures.pop_back();
}
