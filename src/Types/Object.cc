#include <cassert>
#include <iostream>
#include <map>

#include "Utils.h"
#include "debug/alert.h"

#include "Token.h"
#include "Object.h"

#include "AST.h"

// --------------------------------------------------------
//  Object
// --------------------------------------------------------

static bool nested = 0;

bool Object::equals(Object* object) const
{
#define eeeee(A, B) \
  case TYPE_##A:    \
    return ((Obj##B*)this)->equals((Obj##B*)object)

#define ajjja(A) eeeee(A, A)

  if (!this->type.equals(object->type))
    return 0;

  // なんなのこれ
  // めんどくさすぎ
  switch (this->type.kind) {
    eeeee(Int, Long);

    ajjja(Float);
    ajjja(Bool);
    ajjja(Char);
    ajjja(String);
    ajjja(Dict);
    ajjja(Vector);

    case TYPE_None:
      return true;
  }

  todo_impl;
}

std::string ObjUserType::to_string() const
{
  auto pStruct = (AST::Struct*)this->type.userdef_type;

  auto ret = std::string(pStruct->name) + "{ ";

  auto ns = nested;
  nested = true;

  for (auto pm = pStruct->members.begin(); auto&& member : this->members) {
    ret += std::string((pm++)->name) + ": " + member->to_string();

    if (pm != pStruct->members.end())
      ret += ", ";
  }

  nested = ns;
  return ret + " }";
}

std::string ObjNone::to_string() const
{
  return "none";
}

std::string ObjLong::to_string() const
{
  return std::to_string(this->value);
}

std::string ObjUSize::to_string() const
{
  return std::to_string(this->value);
}

std::string ObjFloat::to_string() const
{
  auto ret = std::to_string(this->value);

  while (ret.size() > 1 && *ret.rbegin() == '0') {
    ret.pop_back();
  }

  return ret;
}

std::string ObjString::to_string() const
{
  if (nested) {
    return '"' + Utils::String::to_str(this->value) + '"';
  }

  return Utils::String::to_str(this->value);
}

std::string ObjRange::to_string() const
{
  return Utils::format("%zd..%zd", this->begin, this->end);
}

std::string ObjDict::to_string() const
{
  std::string s = "{";

  auto nss = nested;
  nested = 1;

  for (auto&& x : this->items) {
    s += x.key->to_string() + ": " + x.value->to_string();

    if (&x != &*this->items.rbegin())
      s += ", ";
  }

  nested = nss;

  return s + "}";
}

std::string ObjVector::to_string() const
{
  std::string s = "[";

  auto nss = nested;
  nested = 1;

  for (auto&& x : this->elements) {
    s += x->to_string();
    if (x != *this->elements.rbegin())
      s += ", ";
  }

  nested = nss;

  return s + "]";
}

ObjUserType* ObjUserType::clone() const
{
  auto obj = new ObjUserType(this->type);

  for (auto&& member : this->members)
    obj->add_member(member->clone());

  return obj;
}

ObjNone* ObjNone::clone() const
{
  return new ObjNone;  // ???
}

ObjLong* ObjLong::clone() const
{
  return new ObjLong(this->value);
}

ObjUSize* ObjUSize::clone() const
{
  return new ObjUSize(this->value);
}

ObjFloat* ObjFloat::clone() const
{
  return new ObjFloat(this->value);
}

ObjString* ObjString::clone() const
{
  return new ObjString(this->value);
}

ObjRange* ObjRange::clone() const
{
  return new ObjRange(this->begin, this->end);
}

ObjDict* ObjDict::clone() const
{
  auto ret = new ObjDict;

  for (auto&& item : this->items) {
    ret->append(item.key->clone(), item.value->clone());
  }

  return ret;
}

ObjVector* ObjVector::clone() const
{
  auto ret = new ObjVector;

  for (auto&& xx : this->elements)
    ret->append(xx->clone());

  return ret;
}