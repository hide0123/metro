#include <iostream>
#include "metro.h"

static Object* print_impl(std::vector<Object*>&& args) {
  size_t len = 0;

  for( auto&& arg : args ) {
    auto const& s = arg->to_string();

    std::cout << s;

    len += s.length();
  }

  return new ObjLong(len);
}

static std::vector<BuiltinFunc> const _builtin_functions {
  // print
  BuiltinFunc {
    .name = "print",
    .result_type = TYPE_Int,
    .arg_types = { TYPE_Args },
    .impl = print_impl
  },

  // println
  BuiltinFunc {
    .name = "println",
    .result_type = TYPE_Int,
    .arg_types = { TYPE_Args },
    .impl = [] (std::vector<Object*>&& args) -> Object* {
      auto ret = print_impl(std::move(args));

      std::cout << "\n";
      ((ObjLong*)ret)->value += 1;

      return ret;
    }
  }
};

std::vector<BuiltinFunc> const& BuiltinFunc::get_builtin_list() {
  return ::_builtin_functions;
}

