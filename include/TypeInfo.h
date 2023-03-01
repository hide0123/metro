#pragma once

#include <string>
#include <vector>

// ---------------------------------------------
//  TypeInfo
// ---------------------------------------------
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
  TYPE_Template,  // only used in Sema
};

struct TypeInfo {
  TypeKind kind;
  bool is_const;

  std::vector<TypeInfo> type_params;

  TypeInfo(TypeKind kind = TYPE_None)
      : kind(kind),
        is_const(false)
  {
  }

  TypeInfo(TypeKind kind, std::initializer_list<TypeInfo> list,
           bool is_const = false)
      : kind(kind),
        is_const(is_const),
        type_params(list)
  {
  }

  static std::vector<std::string> const& get_name_list();

  std::string to_string() const;

  bool equals(TypeInfo const& type) const;

  bool is_numeric() const;
};
