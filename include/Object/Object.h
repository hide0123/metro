// ---------------------------------------------
//  Object
// ---------------------------------------------
#pragma once

#include "TypeInfo.h"
#include "AST/ASTfwd.h"
#include "mt_string.h"

struct ObjLong;
struct ObjString;

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

  ObjLong* as_long() const
  {
    return (ObjLong*)this;
  }

  bool equals(Object* object) const;

  virtual ~Object();

protected:
  Object(TypeInfo type);
};

template <std::derived_from<Object> T>
T*& add_refcount(T*& obj)
{
  obj->ref_count++;
  return obj;
}

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

  explicit ObjUserType(AST::Typeable* ast)
    : Object(TYPE_UserDef)
  {
    this->type.userdef_type = ast;
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

struct ObjEnumerator : Object {
  size_t index;
  Object* value;

  std::string to_string() const;
  ObjEnumerator* clone() const;

  bool is_numeric() const
  {
    return false;
  }

  bool equals(ObjEnumerator* x) const
  {
    return this->index == x->index &&
           (this->value ? this->value->equals(x->value) : true);
  }

  Object*& set_value(Object* val)
  {
    val->ref_count++;
    return this->value = val;
  }

  ObjEnumerator(AST::Typeable* enum_ast, size_t index = 0,
                Object* value = nullptr)
    : Object(TYPE_Enumerator),
      index(index),
      value(value)
  {
    this->type.userdef_type = enum_ast;
  }

  ~ObjEnumerator()
  {
    if (this->value)
      this->value->ref_count--;
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
  metro_char_t value;

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
  std::vector<ObjChar*> characters;

  std::string to_string() const;
  ObjString* clone() const;

  bool equals(ObjString* x) const
  {
    if (this->characters.size() == x->characters.size()) {
      for (auto it = this->characters.begin(); auto&& c : x->characters)
        if (!(*it)->equals(c))
          return false;
    }

    return false;
  }

  ObjChar*& append(ObjChar* c)
  {
    return add_refcount(this->characters.emplace_back(c));
  }

  ObjChar*& append(metro_char_t c)
  {
    return add_refcount(this->characters.emplace_back(new ObjChar(c)));
  }

  // copy
  ObjString& append(ObjString* str)
  {
    for (auto&& c : str->characters) {
      this->append(c->clone());
    }

    return *this;
  }

  ObjString& append(std::wstring const& str)
  {
    for (auto&& c : str)
      this->append(c);

    return *this;
  }

  metro_string_t get_string() const
  {
    metro_string_t ret;

    for (auto&& c : this->characters)
      ret += c->value;

    return ret;
  }

  size_t length() const
  {
    return this->characters.size();
  }

  static ObjString* from_u8_string(std::string const& str)
  {
    return new ObjString(Utils::String::to_wstr(str));
  }

  ObjString(std::wstring const& value = L"")
    : Object(TYPE_String)
  {
    for (auto&& c : value)
      this->append(c);
  }

  ObjString(std::vector<ObjChar*> const& vec)
    : Object(TYPE_String),
      characters(vec)
  {
    for (auto&& c : this->characters)
      c->ref_count--;
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
      this->key->ref_count++;
      this->value->ref_count++;
    }

    ~Item()
    {
      this->key->ref_count--;
      this->value->ref_count--;
    }
  };

  std::vector<Item> items;

  std::string to_string() const;
  ObjDict* clone() const;

  bool equals(ObjDict* x) const
  {
    if (this->items.size() != x->items.size())
      return false;

    for (auto xx = x->items.begin(); auto&& self : this->items) {
      if (!self.key->equals(xx->key) || !self.value->equals(xx->value))
        return false;
      xx++;
    }

    return true;
  }

  Item& append(Object* key, Object* value)
  {
    return this->items.emplace_back(key, value);
  }

  ObjDict()
    : Object(TYPE_Dict)
  {
  }

  ObjDict(std::vector<Item>&& _items)
    : Object(TYPE_Dict),
      items(std::move(_items))
  {
  }

  ~ObjDict()
  {
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

    for (auto y = x->elements.begin(); auto&& e : this->elements) {
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
