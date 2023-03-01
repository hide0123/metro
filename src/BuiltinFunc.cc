#include <iostream>

#include "Object.h"
#include "BuiltinFunc.h"

static Object* print_impl(std::vector<Object*> const& args)
{
  size_t len = 0;

  for (auto&& arg : args) {
    auto s = arg->to_string();

    std::cout << s;

    len += s.length();
  }

  return new ObjLong(len);
}

static std::vector<BuiltinFunc> const _builtin_functions{
    // print
    BuiltinFunc{.name = "print",
                .is_template = false,
                .result_type = TYPE_Int,
                .arg_types = {TYPE_Args},
                .impl = print_impl},

    // println
    BuiltinFunc{
        .name = "println",
        .is_template = false,
        .result_type = TYPE_Int,
        .arg_types = {TYPE_Args},
        .impl = [](std::vector<Object*> const& args) -> Object* {
          auto ret = print_impl(args);

          std::cout << "\n";
          ((ObjLong*)ret)->value += 1;

          return ret;
        }},

    // push
    BuiltinFunc{
        .name = "push",
        .is_template = true,
        .result_type = TYPE_None,
        .arg_types = {TypeInfo(TYPE_Vector, {TYPE_Template}),
                      TYPE_Template},
        .impl = [](std::vector<Object*> const& args) -> Object* {
          ((ObjVector*)args[0])->elements.emplace_back(args[1]);
          return new ObjNone;
        }}};

std::vector<BuiltinFunc> const& BuiltinFunc::get_builtin_list()
{
  return ::_builtin_functions;
}
