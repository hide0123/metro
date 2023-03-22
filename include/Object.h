// ---------------------------------------------
//  Object
// ---------------------------------------------
#pragma once

#include "TypeInfo.h"

struct Object {
  TypeInfo type;
  size_t ref_count;
  bool no_delete;

  virtual Object* clone() const = 0;
  virtual std::string to_string() const = 0;

  virtual bool is_numeric() const
  {
    return false;
  }

  bool equals(Object* object) const;

  virtual ~Object();

protected:
  Object(TypeInfo type);
};

struct ObjUserType : Object {
  std::vector<Object*> members;

  std::string to_string() const;
  ObjUserType* clone() const;

  Object*& add_member(Object* obj)
  {
    auto& ret = this->members.emplace_back(obj);

    ret->ref_count++;

    return ret;
  }

  explicit ObjUserType(TypeInfo const& type)
      : Object(type)
  {
  }

  ~ObjUserType()
  {
    for (auto&& m : this->members) {
      m->ref_count--;
    }
  }
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

  bool is_numeric() const
  {
    return true;
  }

  bool equals(ObjLong* x) const
  {
    return this->value == x->value;
  }

  explicit ObjLong(int64_t value = 0)
      : Object(TYPE_Int),
        value(value)
  {
  }
};

struct ObjUSize : Object {
  size_t value;

  std::string to_string() const;
  ObjUSize* clone() const;

  bool is_numeric() const
  {
    return true;
  }

  bool equals(ObjUSize* x) const
  {
    return this->value == x->value;
  }

  ObjUSize(size_t value = 0)
      : Object(TYPE_USize),
        value(value)
  {
  }
};

struct ObjFloat : Object {
  float value;

  std::string to_string() const;
  ObjFloat* clone() const;

  bool is_numeric() const
  {
    return true;
  }

  bool equals(ObjFloat* x) const
  {
    return this->value == x->value;
  }

  explicit ObjFloat(float value = 0)
      : Object(TYPE_Float),
        value(value)
  {
  }
};

struct ObjBool : Object {
  bool value;

  std::string to_string() const
  {
    return value ? "true" : "false";
  }

  ObjBool* clone() const
  {
    return new ObjBool(this->value);
  }

  bool equals(ObjBool* x) const
  {
    return this->value == x->value;
  }

  explicit ObjBool(bool value = 0)
      : Object(TYPE_Bool),
        value(value)
  {
  }
};

struct ObjChar : Object {
  wchar_t value;

  std::string to_string() const;
  ObjChar* clone() const;

  bool equals(ObjChar* x) const
  {
    return this->value == x->value;
  }

  explicit ObjChar(wchar_t ch = 0)
      : Object(TYPE_Char),
        value(ch)
  {
  }
};

struct ObjString : Object {
  std::wstring value;

  std::string to_string() const;
  ObjString* clone() const;

  bool equals(ObjString* x) const
  {
    return this->value == x->value;
  }

  ObjString(std::wstring const& value = L"")
      : Object(TYPE_String),
        value(value)
  {
  }

  ObjString(std::wstring&& value)
      : Object(TYPE_String),
        value(std::move(value))
  {
  }
};

struct ObjRange : Object {
  int64_t begin;
  int64_t end;

  std::string to_string() const;
  ObjRange* clone() const;

  bool equals(ObjRange* x) const
  {
    return this->begin == x->begin && this->end == x->end;
  }

  ObjRange(int64_t begin, int64_t end)
      : Object(TYPE_Range),
        begin(begin),
        end(end)
  {
  }
};

struct ObjDict : Object {
  struct Item {
    Object* key;
    Object* value;

    Item(Object* k, Object* v)
        : key(k),
          value(v)
    {
    }
  };

  std::vector<Item> items;

  std::string to_string() const;
  ObjDict* clone() const;

  bool equals(ObjDict* x) const
  {
    if (this->items.size() != x->items.size())
      return false;

    for (auto xx = x->items.begin();
         auto&& aa : this->items) {
      if (!aa.key->equals(xx->key) ||
          !aa.value->equals(xx->value))
        return false;
      xx++;
    }

    return true;
  }

  Item& append(Object* key, Object* value)
  {
    auto& item = this->items.emplace_back(key, value);

    item.key->ref_count++;
    item.value->ref_count++;

    return item;
  }

  ObjDict()
      : Object(TYPE_Dict)
  {
  }

  ObjDict(std::vector<Item>&& _items)
      : Object(TYPE_Dict),
        items(std::move(_items))
  {
    for (auto&& item : this->items) {
      item.key->ref_count++;
      item.value->ref_count++;
    }
  }

  ~ObjDict()
  {
    for (auto&& item : this->items) {
      item.key->ref_count--;
      item.value->ref_count--;
    }
  }
};

struct ObjVector : Object {
  std::vector<Object*> elements;

  std::string to_string() const;
  ObjVector* clone() const;

  bool equals(ObjVector* x) const
  {
    if (this->elements.size() != x->elements.size())
      return false;

    for (auto y = x->elements.begin();
         auto&& e : this->elements) {
      if (!e->equals(*y))
        return false;
    }

    return true;
  }

  Object*& append(Object* obj)
  {
    auto& ret = this->elements.emplace_back(obj);

    ret->ref_count++;

    return ret;
  }

  ObjVector()
      : Object(TYPE_Vector)
  {
  }

  ObjVector(std::vector<Object*>&& elems)
      : Object(TYPE_Vector),
        elements(std::move(elems))
  {
    for (auto&& e : this->elements) {
      e->ref_count++;
    }
  }

  ~ObjVector()
  {
    for (auto&& elem : this->elements) {
      elem->ref_count--;
    }
  }
};
