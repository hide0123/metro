#include <cassert>
#include <iostream>
#include "lcc.h"

// --------------------------------------------------------
//  Object
// --------------------------------------------------------

void Object::initialize() {
  Object::obj_none = new ObjNone;
}

std::string ObjNone::to_string() const {
  return "none";
}

std::string ObjLong::to_string() const {
  return std::to_string(this->value);
}


// --------------------------------------------------------
//  Error
// --------------------------------------------------------

Error& Error::emit() {
  std::cerr << this->msg << std::endl;

  return *this;
}

void Error::exit(int code) {
  std::exit(code);
}

// --------------------------------------------------------
//  TypeInfo
// --------------------------------------------------------

//
// TypeInfo
// 文字列に変換
std::string TypeInfo::to_string() const {
  static std::map<TypeKind, char const*> kind_name_map {
    { TYPE_Int, "int" },
    { TYPE_Float, "float" },
    { TYPE_Char, "char" },
    { TYPE_String, "string" },
  };

  assert(kind_name_map.contains(this->kind));

  std::string s = kind_name_map[this->kind];

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

// --------------------------------------------------------
