#pragma once

#include <string>
#include <vector>

// ---------------------------------------------
//  TypeInfo
// ---------------------------------------------
enum TypeKind {
  TYPE_None,
  TYPE_Int,
  TYPE_Float,
  TYPE_Bool,
  TYPE_Char,
  TYPE_String,
  TYPE_Vector,
  TYPE_Args,
};

struct TypeInfo {
  TypeKind kind;
  bool is_mutable;

  std::vector<TypeInfo> type_params;

  TypeInfo(TypeKind kind = TYPE_None)
    : kind(kind),
      is_mutable(false)
  {
  }

  static std::vector<std::string> const& get_name_list();

  std::string to_string() const;

  bool equals(TypeInfo const& type) const;

  bool is_numeric() const;
};
