#include <cassert>
#include <iostream>

#include "common.h"

#include "Token.h"
#include "Object.h"

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
  alert_dtor;
}

void Object::initialize() {


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
