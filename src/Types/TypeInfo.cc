#include <cassert>
#include <iostream>

#include "Utils.h"
#include "debug/alert.h"

#include "Token.h"
#include "TypeInfo.h"
#include "AST.h"

static std::vector<std::pair<TypeKind, char const*>> const g_kind_and_names{
    {TYPE_None, "none"},     {TYPE_Int, "int"},     {TYPE_USize, "usize"},
    {TYPE_Float, "float"},   {TYPE_Bool, "bool"},   {TYPE_Char, "char"},
    {TYPE_String, "string"}, {TYPE_Range, "range"}, {TYPE_Vector, "vector"},
    {TYPE_Dict, "dict"},     {TYPE_Args, "args"},
};

//
// ------------------------------------------------
//  Get g_kind_and_names
// ------------------------------------------------
std::vector<std::pair<TypeKind, char const*>> const&
TypeInfo::get_kind_and_names()
{
  return g_kind_and_names;
}

//
// ------------------------------------------------
//  Create a name list of all types
// ------------------------------------------------
std::vector<std::string> TypeInfo::get_name_list()
{
  std::vector<std::string> ret;

  for (auto&& [kind, name] : g_kind_and_names)
    ret.emplace_back(name);

  return ret;
}

std::optional<TypeKind> TypeInfo::get_kind_from_name(
    std::string_view const& name)
{
  for (auto&& [kind, n] : g_kind_and_names)
    if (n == name)
      return kind;

  return std::nullopt;
}

//
// ------------------------------------------------
//  Convert TypeInfo to std::string
// ------------------------------------------------
std::string TypeInfo::to_string() const
{
  if (this->kind == TYPE_UserDef) {
    return std::string(this->userdef_struct->name);
  }

  std::string s = ::g_kind_and_names[static_cast<int>(this->kind)].second;

  //
  // template-parameters
  if (!this->type_params.empty()) {
    s += "<";

    for (auto&& x : this->type_params) {
      s += x.to_string();
      if (&x != &*this->type_params.rbegin())
        s += ", ";
    }

    s += ">";
  }

  //
  // const
  if (this->is_const) {
    s += " const";
  }

  return s;
}

//
// TypeInfo
// 同じかどうか比較する
bool TypeInfo::equals(TypeInfo const& type) const
{
  if (this->kind == TYPE_Template || type.kind == TYPE_Template)
    return true;

  if (this->kind != type.kind)
    return false;

  //
  // user-defined
  if (this->kind == TYPE_UserDef) {
    if (this->userdef_struct != type.userdef_struct)
      return false;
  }

  //
  // size of parameters
  if (this->type_params.size() != type.type_params.size())
    return false;

  for (auto self_iter = this->type_params.begin();
       auto&& tparam : type.type_params) {
    if (!(self_iter++)->equals(tparam))
      return false;
  }

  return true;
}