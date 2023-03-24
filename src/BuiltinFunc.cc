#include <iostream>

#include "Utils.h"
#include "debug/alert.h"

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
  // id
  BuiltinFunc{.name = "id",
              .is_template = true,
              .have_self = false,
              .is_template = true,
              .result_type = TYPE_Int,
              .arg_types = {TYPE_Template},
              .impl = [](std::vector<Object*> const& args) -> Object* {
                return new ObjString(
                  Utils::String::to_wstr(Utils::format("%p", args[0])));
              }},

  // length
  BuiltinFunc{.name = "length",
              .is_template = true,
              .result_type = TYPE_USize,
              .arg_types = {TYPE_Template},
              .impl = [](std::vector<Object*> const& args) -> Object* {
                auto obj = args[0];

                switch (obj->type.kind) {
                  case TYPE_String:
                    return new ObjUSize(((ObjString*)obj)->characters.size());

                  case TYPE_Vector:
                    return new ObjUSize(((ObjVector*)obj)->elements.size());

                  case TYPE_Dict:
                    return new ObjUSize(((ObjDict*)obj)->items.size());
                }
              }},

  // print
  BuiltinFunc{.name = "print",
              .is_template = false,
              .result_type = TYPE_Int,
              .arg_types = {TYPE_Args},
              .impl = print_impl},

  // println
  BuiltinFunc{.name = "println",
              .is_template = false,
              .result_type = TYPE_Int,
              .arg_types = {TYPE_Args},
              .impl = [](std::vector<Object*> const& args) -> Object* {
                auto ret = print_impl(args);

                std::cout << "\n";
                ((ObjLong*)ret)->value += 1;

                return ret;
              }},

  BuiltinFunc{.name = "input",
              .is_template = false,
              .result_type = TYPE_String,
              .arg_types = {},
              .impl = [](std::vector<Object*> const& args) -> Object* {
                (void)args;

                std::string input;

                std::getline(std::cin, input);

                return new ObjString(Utils::String::to_wstr(input));
              }},

  // push
  BuiltinFunc{
    .name = "push",
    .is_template = true,
    .result_type = TYPE_None,
    .arg_types = {TypeInfo(TYPE_Vector, {TYPE_Template}), TYPE_Template},
    .impl = [](std::vector<Object*> const& args) -> Object* {
      return ((ObjVector*)args[0])->append(args[1]);
    }},

  // to_string
  BuiltinFunc{.name = "to_string",
              .is_template = true,
              .result_type = TYPE_String,
              .arg_types = {TYPE_Template},
              .impl = [](std::vector<Object*> const& args) -> Object* {
                return new ObjString(
                  Utils::String::to_wstr(args[0]->to_string()));
              }},

  // type
  BuiltinFunc{.name = "type",
              .is_template = true,
              .result_type = TYPE_String,
              .arg_types = {TYPE_Template},
              .impl = [](std::vector<Object*> const& args) -> Object* {
                return new ObjString(
                  Utils::String::to_wstr(args[0]->type.to_string()));
              }},

  // exit
  BuiltinFunc{.name = "exit",
              .is_template = false,
              .result_type = TYPE_None,
              .arg_types = {TYPE_Int},
              .impl = [](std::vector<Object*> const& args) -> Object* {
                std::exit((int)((ObjLong*)args[0])->value);
              }},

};

std::vector<BuiltinFunc> const& BuiltinFunc::get_builtin_list()
{
  return ::_builtin_functions;
}
