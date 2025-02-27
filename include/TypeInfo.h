// ---------------------------------------------
//  TypeInfo
// ---------------------------------------------

#pragma once

#include <string>
#include <vector>
#include <optional>

namespace AST {

struct Typeable;

}

enum TypeKind {
  TYPE_None,
  TYPE_Int,
  TYPE_USize,
  TYPE_Enumerator,
  TYPE_Float,
  TYPE_Bool,
  TYPE_Char,
  TYPE_String,
  TYPE_Range,
  TYPE_Vector,
  TYPE_Dict,
  TYPE_Args,
  TYPE_UserDef,  // user-defined
  TYPE_Template,  // only used in Sema
};

struct TypeInfo {
  using member_pair_t = std::pair<std::string_view, TypeInfo>;

  TypeKind kind;
  bool is_const;

  //
  // template parameters
  std::vector<TypeInfo> type_params;

  //
  // member variables
  std::vector<member_pair_t> members;

  //
  // if type is struct, this is a pointer to it.
  AST::Typeable* userdef_type;

  TypeInfo(TypeKind kind = TYPE_None)
    : kind(kind),
      is_const(false),
      userdef_type(nullptr)
  {
  }

  TypeInfo(TypeKind kind, std::initializer_list<TypeInfo> list,
           bool is_const = false)
    : kind(kind),
      is_const(is_const),
      type_params(list)
  {
  }

  std::string to_string() const;

  bool equals(TypeInfo const& type) const;

  bool have_members() const
  {
    if (this->kind == TYPE_UserDef)
      return true;

    return false;
  }

  bool have_params() const
  {
    switch (this->kind) {
      case TYPE_Vector:
      case TYPE_Dict:
        return true;
    }

    return false;
  }

  //
  // 自身が数値型であるかどうか
  //
  // 引数:
  //  is_only_integer   整数型以外の数値型を対象外とする
  //
  // 戻り値:
  //  数値型であれば true
  //  そうでなければ false
  bool is_numeric(bool is_only_integer = false) const
  {
    switch (this->kind) {
      case TYPE_Int:
      case TYPE_USize:
        return true;

      case TYPE_Float:
        if (!is_only_integer)
          return true;
    }

    return false;
  }

  bool is_iterable() const
  {
    switch (this->kind) {
      case TYPE_Range:
      case TYPE_Vector:
      case TYPE_Dict:
        return true;
    }

    return false;
  }

  static std::vector<std::pair<TypeKind, char const*>> const&
  get_kind_and_names();

  static std::vector<std::string> get_name_list();

  static std::optional<TypeKind> get_kind_from_name(
    std::string_view const& name);

  int find_member(std::string_view const& name)
  {
    for (int i = 0; auto&& [n, t] : this->members) {
      if (n == name)
        return i;

      i++;
    }

    return -1;
  }

  static TypeInfo from_usertype(AST::Typeable* type)
  {
    TypeInfo ret = TYPE_UserDef;
    ret.userdef_type = type;

    return ret;
  }
};
