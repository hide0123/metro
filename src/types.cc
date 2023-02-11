#include <cassert>
#include <iostream>
#include "metro.h"

// --------------------------------------------------------
//  Object
// --------------------------------------------------------

ObjNone* Object::obj_none;

Object::Object(TypeInfo type)
  : type(type),
    ref_count(0)
{
}

Object::~Object() {

}

void Object::initialize() {
  Object::obj_none = new ObjNone;
}

std::string ObjNone::to_string() const {
  return "none";
}

std::string ObjLong::to_string() const {
  return std::to_string(this->value);
}

std::string ObjFloat::to_string() const {
  // 処理まったく上と同じだからどうにかしろ
  return std::to_string(this->value);
}

std::string ObjString::to_string() const {
  return Utils::String::to_str(this->value);
}

ObjNone* ObjNone::clone() const {
  return new ObjNone; // ???
}

ObjLong* ObjLong::clone() const {
  return new ObjLong(this->value);
}

ObjFloat* ObjFloat::clone() const {
  return new ObjFloat(this->value);
}

ObjString* ObjString::clone() const {
  return new ObjString(this->value);
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

static std::vector<std::string> const all_type_names {
  "none",
  "int",
  "float",
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
    < ::all_type_names.size());

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

// --------------------------------------------------------
