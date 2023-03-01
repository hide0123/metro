#pragma once

#include "TypeInfo.h"

// ---------------------------------------------
//  Object
// ---------------------------------------------

class Application;
struct ObjNone;
struct Object {
  TypeInfo type;
  size_t ref_count;

  static ObjNone* obj_none;

  static void initialize();

  virtual std::string to_string() const = 0;

  virtual Object* clone() const = 0;

  virtual bool is_numeric() const {
    return false;
  }

  virtual bool equals(Object* object) const = 0;

  virtual ~Object();

protected:
  Object(TypeInfo type);

  friend class Application;
};

struct ObjNone : Object {
  std::string to_string() const;
  ObjNone* clone() const;

  explicit ObjNone()
    : Object(TYPE_None)
  {
  }
};

struct ObjLong : Object {
  int64_t value;

  std::string to_string() const;
  ObjLong* clone() const;

  bool equals(ObjLong* x) const {
    return x->value==this->value;
  }

  bool is_numeric() const {
    return true;
  }

  explicit ObjLong(int64_t value)
    : Object(TYPE_Int),
      value(value)
  {
  }
};

struct ObjUSize : Object {
  size_t value;

  std::string to_string() const;
  ObjLong* clone() const;

  bool equals(ObjUSize* x) const {
    return x->value==this->value;
  }

  bool is_numeric() const {
    return true;
  }

  ObjUSize(size_t value)
    : Object(TYPE_USize),
      value(value)
  {
  }
};

struct ObjFloat : Object {
  float value;

  std::string to_string() const;
  ObjFloat* clone() const;

  bool equals(ObjFloat* x) const {
    return x->value==this->value;
  }

  bool is_numeric() const {
    return true;
  }

  explicit ObjFloat(float value)
    : Object(TYPE_Float),
      value(value)
  {
  }
};

struct ObjBool : Object {
  bool value;

  std::string to_string() const {
    return value ? "true" : "false";
  }

  ObjBool* clone() const {
    return new ObjBool(this->value);
  }

  bool equals(ObjBool* x) const {
    return x->value==this->value;
  }

  explicit ObjBool(bool value)
    : Object(TYPE_Bool),
      value(value)
  {
  }
};

struct ObjString : Object {
  std::wstring value;

  std::string to_string() const;
  ObjString* clone() const;

  bool equals(ObjString* x) const {
    return x->value==this->value;
  }

  explicit ObjString(std::wstring value = L"")
    : Object(TYPE_String),
      value(value)
  {
  }
};

struct ObjDict : Object {
  struct Item {
    Object* key;
    Object* value;

    Item(Object* k, Object* v) :key(k), value(v) { }
  };

  std::vector<Item> items;

  std::string to_string() const;
  ObjDict* clone() const;

  bool equals(ObjDict* x) const {
    if(this->items.size()!=x->items.size())
      return false;

    for(auto xx=x->items.begin();auto&&aa:this->items){
      if(!aa.key->equals(xx->key) || !aa.value->equals(xx->value))
        return false;
    }

    return true;
  }

  ObjDict()
    : Object(TYPE_Dict)
  {
  }
};

struct ObjVector : Object {
  std::vector<Object*> elements;

  std::string to_string() const;
  ObjVector* clone() const;

  bool equals(ObjVector* x) const {

  }

  ObjVector()
    : Object(TYPE_Vector)
  {
  }
};

