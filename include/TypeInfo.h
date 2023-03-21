// ---------------------------------------------
//  TypeInfo
// ---------------------------------------------

#pragma once

#include <string>
#include <vector>
#include <optional>

namespace AST {

struct Struct;

}

enum TypeKind {
  TYPE_None,
  TYPE_Int,
  TYPE_USize,
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
  using member_pair_t =
      std::pair<std::string_view, TypeInfo>;

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
  AST::Struct* userdef_struct;

  TypeInfo(TypeKind kind = TYPE_None)
      : kind(kind),
        is_const(false),
        userdef_struct(nullptr)
  {
  }

  TypeInfo(TypeKind kind,
           std::initializer_list<TypeInfo> list,
           bool is_const = false)
      : kind(kind),
        is_const(is_const),
        type_params(list)
  {
  }

  static std::vector<
      std::pair<TypeKind, char const*>> const&
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

  std::string to_string() const;

  bool equals(TypeInfo const& type) const;

  bool have_members() const;

  bool is_numeric() const;
  bool is_iterable() const;
};
