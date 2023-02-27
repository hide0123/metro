#include <cassert>
#include <iostream>

#include "common.h"

#include "Token.h"
#include "TypeInfo.h"

static std::vector<std::string> const all_type_names {
  "none",
  "int",
  "float",
  "bool",
  "char",
  "string",
  "vector",
  "args"
};

std::vector<std::string> const& TypeInfo::get_name_list() {
  return ::all_type_names;
}

//
// TypeInfo
// 文字列に変換
std::string TypeInfo::to_string() const {
  assert(static_cast<int>(this->kind)
    < (int)::all_type_names.size());

  std::string s =
    ::all_type_names[static_cast<int>(this->kind)];

  if( this->is_mutable ) {
    s += " mut";
  }

  return s;
}

//
// TypeInfo
// 同じかどうか比較する
bool TypeInfo::equals(TypeInfo const& type) const {
  if(
    this->kind != type.kind
    || this->is_mutable != type.is_mutable )
      return false;

  if( !this->type_params.empty()
    && this->type_params.size() == type.type_params.size() ) {
    for( auto self_iter = this->type_params.begin();
        auto&& tparam : type.type_params ) {
      if( !(self_iter++)->equals(tparam) )
        return false;
    }
  }

  return true;
}

bool TypeInfo::is_numeric() const {
  return
    this->kind == TYPE_Int || this->kind == TYPE_Float;
}
