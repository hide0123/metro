#include <cassert>
#include <iostream>

#include "common.h"

#include "Token.h"
#include "TypeInfo.h"

#include "color.h"
#include "debug/alert.h"

static std::vector<std::string> const all_type_names{
    "none",   "int",   "usize",  "float", "bool", "char",
    "string", "range", "vector", "dict",  "args", "any"};

std::vector<std::string> const& TypeInfo::get_name_list()
{
  return ::all_type_names;
}

//
// TypeInfo
// 文字列に変換
std::string TypeInfo::to_string() const
{
  alertmsg("this->kind = " << this->kind);

  assert(static_cast<int>(this->kind) <
         (int)::all_type_names.size());

  std::string s = ::all_type_names[static_cast<int>(this->kind)];

  if (!this->type_params.empty()) {
    s += "<";

    for (auto&& x : this->type_params) {
      s += x.to_string();
      if (&x != &*this->type_params.rbegin())
        s += ", ";
    }

    s += ">";
  }

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

  if (this->kind != type.kind || this->is_const != type.is_const)
    return false;

  if (this->type_params.size() != type.type_params.size())
    return false;

  for (auto self_iter = this->type_params.begin();
       auto&& tparam : type.type_params) {
    if (!(self_iter++)->equals(tparam))
      return false;
  }

  return true;
}

bool TypeInfo::is_numeric() const
{
  switch (this->kind) {
    case TYPE_Int:
    case TYPE_USize:
    case TYPE_Float:
      return true;
  }

  return false;
}

bool TypeInfo::is_iterable() const
{
  switch (this->kind) {
    case TYPE_Range:
    case TYPE_Vector:
    case TYPE_Dict:
      return true;
  }

  return false;
}