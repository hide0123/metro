#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Sema.h"

std::optional<TypeInfo> Sema::get_type_from_name(
    std::string_view name)
{
  if (auto builtin = TypeInfo::get_kind_from_name(name);
      builtin)
    return builtin.value();

  if (auto usrdef = this->find_struct(name); usrdef) {
    TypeInfo ret{TYPE_UserDef};

    ret.userdef_struct = usrdef;

    for (auto&& member : usrdef->members) {
      ret.members.emplace_back(member.name,
                               this->check(member.type));
    }

    return ret;
  }

  return std::nullopt;
}

AST::Function* Sema::find_function(std::string_view name)
{
  for (auto&& item : this->root->list)
    if (item->kind == AST_Function &&
        ((AST::Function*)item)->name.str == name)
      return (AST::Function*)item;

  return nullptr;
}

AST::Struct* Sema::find_struct(std::string_view name)
{
  for (auto&& item : this->root->list) {
    if (auto st = (AST::Struct*)item;
        st->kind == AST_Struct && st->name == name) {
      return st;
    }
  }

  return nullptr;
}

BuiltinFunc const* Sema::find_builtin_func(
    std::string_view name)
{
  for (auto&& builtinfunc : BuiltinFunc::get_builtin_list())
    if (builtinfunc.name == name)
      return &builtinfunc;

  return nullptr;
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

TypeInfo Sema::expect(TypeInfo const& expected,
                      AST::Base* ast)
{
  auto type = this->check(ast);

  if (type.equals(expected))
    return expected;

  switch (type.kind) {
    case TYPE_Vector: {
      auto x = (AST::Vector*)ast;

      if (x->elements.empty())
        return expected;

      break;
    }

    case TYPE_Dict: {
      if (ast->kind == AST_Scope &&
          ((AST::Scope*)ast)->list.empty())
        return expected;

      auto x = (AST::Dict*)ast;

      if (!!x->key_type && x->elements.empty())
        return expected;

      break;
    }
  }

  if (ast->kind == AST_Scope) {
    if (auto x = (AST::Scope*)ast; x->return_last_expr)
      ast = *x->list.rbegin();
  }

  Error(ast, "expected '" + expected.to_string() +
                 "' but found '" + type.to_string() + "'")
      .emit()
      .exit();
}
