#include <cassert>
#include <iostream>
#include <map>

#include "Utils.h"
#include "debug/alert.h"

#include "Token.h"
#include "Object.h"

// --------------------------------------------------------
//  Object
// --------------------------------------------------------

static bool nested = 0;

#if METRO_DEBUG

extern std::map<Object*, int> _all_obj;

void _show_all_obj()
{
  for (auto&& [p, i] : _all_obj) {
    printf("%p ", p);

    if (i) {
      std::cout << p->to_string()
                << Utils::format(" no_delete=%d ref_count=%d",
                                 p->no_delete, p->ref_count);
    }
    else {
      printf(COL_RED "(deleted)" COL_DEFAULT);
    }

    printf("\n");
  }
}

#endif

ObjNone* Object::obj_none;

void Object::initialize()
{
}

bool Object::equals(Object* object) const
{
#define eeeee(A, B) \
  case TYPE_##A:    \
    return ((Obj##B*)this)->equals((Obj##B*)object);

#define ajjja(A) eeeee(A, A)

  if (!this->type.equals(object->type))
    return 0;

  // なんなのこれ
  // めんどくさすぎ
  switch (this->type.kind) {
  eeeee(Int, Long) ajjja(Float) ajjja(Bool) ajjja(Char)
      ajjja(String) ajjja(Dict) ajjja(Vector)

          case TYPE_None:
    return 1;
  }

  todo_impl;
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
  auto&& ret = std::to_string(this->value);

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
    ret->items.emplace_back(item.key->clone(),
                            item.value->clone());
  }

  return ret;
}

ObjVector* ObjVector::clone() const
{
  auto ret = new ObjVector;

  for (auto&& xx : this->elements)
    ret->elements.emplace_back(xx->clone());

  return ret;
}