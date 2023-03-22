// ---------------------------------------------
//  BuiltinFunc
// ---------------------------------------------

#pragma once

#include <functional>
#include <string>
#include <vector>
#include "TypeInfo.h"

struct Object;
struct BuiltinFunc {
  using Implementation =
      std::function<Object*(std::vector<Object*> const&)>;

  std::string name;  // 関数名

  bool is_template;

  TypeInfo result_type;  // 戻り値の型
  std::vector<TypeInfo> arg_types;  // 引数の型

  Implementation impl;  // 処理

  // BuiltinFunc();

  static std::vector<BuiltinFunc> const& get_builtin_list();
};
