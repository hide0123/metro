// ---------------------------------------------
//  BuiltinFunc
// ---------------------------------------------

#pragma once

#include <functional>
#include <string>
#include <vector>
#include "TypeInfo.h"

enum BuiltinKind {
  BLT_Property,
  BLT_Function
};

struct Object;
struct BuiltinFunc {
  struct ArgumentObject {
    Object* object;
    AST::Base* ast;

    explicit ArgumentObject(Object* object, AST::Base* ast)
      : object(object),
        ast(ast)
    {
    }
  };

  using ArgumentVector = std::vector<ArgumentObject>;
  using Implementation = std::function<Object*(ArgumentVector const&)>;

  // BuiltinKind kind;
  std::string name;  // 関数名

  bool is_template;
  bool have_self;

  TypeInfo self_type;  // 使わないかも

  TypeInfo result_type;  // 戻り値の型
  std::vector<TypeInfo> arg_types;  // 引数の型

  Implementation impl;  // 処理

  // BuiltinFunc();

  static std::vector<BuiltinFunc> const& get_builtin_list();
};
