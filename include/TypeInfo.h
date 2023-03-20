// ---------------------------------------------
//  TypeInfo
// ---------------------------------------------

#pragma once

#include <string>
#include <vector>

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
  TypeKind kind;
  bool is_const;

  //
  // template parameters
  std::vector<TypeInfo> type_params;

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

  std::string to_string() const;

  bool equals(TypeInfo const& type) const;

  bool is_numeric() const;
  bool is_iterable() const;
};
