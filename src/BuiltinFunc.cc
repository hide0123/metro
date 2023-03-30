#include <iostream>

#include "Utils.h"
#include "Debug.h"

#include "Object.h"
#include "BuiltinFunc.h"
#include "Error.h"

#define DEFINE_BUILTIN_FUNC(name) \
  static Object* name(BuiltinFunc::ArgumentVector const& args)

#define ImplementLambda [](BuiltinFunc::ArgumentVector const& args) -> Object*

#define BUILTIN_FUNC(Name, IsTemplate, IsHaveSelf, SelfType, ResultType, Impl, \
                     ArgTypes...)                                              \
  BuiltinFunc                                                                  \
  {                                                                            \
    .name = Name, .is_template = IsTemplate                                    \
  }

static Object* print_impl(BuiltinFunc::ArgumentVector const& args)
{
  size_t len = 0;

  for (auto&& arg : args) {
    auto s = arg.object->to_string();

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
              .result_type = TYPE_Int,
              .arg_types = {TYPE_Template},
              .impl = ImplementLambda{return new ObjString(
                Utils::String::to_wstr(Utils::format("%p", args[0])));
}
}
,

  // length
  BuiltinFunc{.name = "length",
              .is_template = true,
              .result_type = TYPE_USize,
              .arg_types = {TYPE_String},
              .impl = ImplementLambda{return new ObjUSize(
                ((ObjString*)args[0].object)->characters.size());
}
}
,

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
              .impl = ImplementLambda{auto ret = print_impl(args);

std::cout << "\n";
((ObjLong*)ret)->value += 1;

return ret;
}
}
,

  BuiltinFunc{.name = "input",
              .is_template = false,
              .result_type = TYPE_String,
              .arg_types = {},
              .impl = ImplementLambda{(void)args;

std::string input;

std::getline(std::cin, input);

return new ObjString(Utils::String::to_wstr(input));
}
}
,

  // push
  BuiltinFunc{.name = "push",
              .is_template = true,
              .have_self = true,
              .self_type = TypeInfo(TYPE_Vector, {TYPE_Template}),
              .result_type = TYPE_None,
              .arg_types = {TYPE_Template},
              .impl = ImplementLambda{
                return ((ObjVector*)args[0].object)->append(args[1].object);
}
}
,

  // to_string
  BuiltinFunc{.name = "to_string",
              .is_template = true,
              .result_type = TYPE_String,
              .arg_types = {TYPE_Template},
              .impl = ImplementLambda{return new ObjString(
                Utils::String::to_wstr(args[0].object->to_string()));
}
}
,

  // substr
  BuiltinFunc{.name = "substr",
              .is_template = false,
              .have_self = true,
              .self_type = TYPE_String,
              .result_type = TYPE_String,
              .arg_types = {TYPE_USize},
              .impl = ImplementLambda{auto ret = (ObjString*)args[0].object;
auto const& index = args[1];

if (((ObjUSize*)(index.object))->value >= ret->characters.size()) {
  Error(index.ast, "index out of range").emit().exit();
}

for (size_t i = 0; i < ((ObjUSize*)index.object)->value; i++) {
  ret->characters.erase(ret->characters.begin());
}

return ret;
}
}
,

  // type
  BuiltinFunc{.name = "type",
              .is_template = true,
              .result_type = TYPE_String,
              .arg_types = {TYPE_Template},
              .impl = ImplementLambda{return new ObjString(
                Utils::String::to_wstr(args[0].object->type.to_string()));
}
}
,

  // exit
  BuiltinFunc{
    .name = "exit",
    .is_template = false,
    .result_type = TYPE_None,
    .arg_types = {TYPE_Int},
    .impl = ImplementLambda{std::exit((int)((ObjLong*)args[0].object)->value);
}
}
,
}
;

std::vector<BuiltinFunc> const& BuiltinFunc::get_builtin_list()
{
  return ::_builtin_functions;
}
