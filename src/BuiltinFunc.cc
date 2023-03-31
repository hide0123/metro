#include <iostream>

#include "Utils.h"
#include "Debug.h"

#include "Object.h"
#include "BuiltinFunc.h"
#include "Error.h"

#define DEFINE_BUILTIN_FUNC(name) \
  static Object* name(BuiltinFunc::ArgumentVector const& args)

#define BUILTIN_FUNC_FULL(Name, Impl, IsTemplate, IsHaveSelf, SelfType,     \
                          ResultType, ArgTypes...)                          \
  BuiltinFunc                                                               \
  {                                                                         \
    .name = Name, .is_template = IsTemplate, .have_self = IsHaveSelf,       \
    .self_type = SelfType, .result_type = ResultType, .arg_types{ArgTypes}, \
    .impl = Impl                                                            \
  }

#define BUILTIN_FUNC(Name, Impl, ResultType, ArgTypes...) \
  BUILTIN_FUNC_FULL(Name, Impl, false, false, ResultType, {}, ArgTypes)

#define BUILTIN_FUNC_TEMPLATE(Name, Impl, ResultType, ArgTypes...) \
  BUILTIN_FUNC_FULL(Name, Impl, true, false, ResultType, {}, ArgTypes)

namespace builtin {

DEFINE_BUILTIN_FUNC(print)
{
  size_t len = 0;

  for (auto&& arg : args) {
    auto s = arg.object->to_string();

    std::cout << s;

    len += s.length();
  }

  return new ObjLong(len);
}

DEFINE_BUILTIN_FUNC(println)
{
  auto ret = print(args);

  std::cout << "\n";
  ((ObjLong*)ret)->value += 1;

  return ret;
}

//
// id(T)
DEFINE_BUILTIN_FUNC(id)
{
  return ObjString::from_u8_string(Utils::format("%p", args[0]));
}

//
// type(T)
DEFINE_BUILTIN_FUNC(type)
{
  return ObjString::from_u8_string(args[0].object->type.to_string());
}

//
// to_string(T)
DEFINE_BUILTIN_FUNC(to_string)
{
  return ObjString::from_u8_string(args[0].object->to_string());
}

//
// length(string)
DEFINE_BUILTIN_FUNC(length)
{
  return new ObjUSize(((ObjString*)args[0].object)->characters.size());
}

//
// push(vector<T>)
DEFINE_BUILTIN_FUNC(push)
{
  return ((ObjVector*)args[0].object)->append(args[1].object);
}

//
// substr
DEFINE_BUILTIN_FUNC(substr)
{
  return ObjString::from_u8_string(args[0].object->to_string());
}

//
// input
DEFINE_BUILTIN_FUNC(input)
{
  std::string input;

  std::getline(std::cin, input);

  return ObjString::from_u8_string(input);
}

//
// exit
DEFINE_BUILTIN_FUNC(exit)
{
  std::exit((int)((ObjLong*)args[0].object)->value);
}

}  // namespace builtin

static std::vector<BuiltinFunc> const _builtin_functions{
  BUILTIN_FUNC("print", builtin::print, TYPE_Int, TYPE_Args),
  BUILTIN_FUNC("println", builtin::println, TYPE_Int, TYPE_Args),

  BUILTIN_FUNC("id", builtin::id, TYPE_Int, TYPE_Template),
  BUILTIN_FUNC("type", builtin::type, TYPE_String, TYPE_Template),

  BUILTIN_FUNC_TEMPLATE("to_string", builtin::to_string, TYPE_String,
                        TYPE_Template),

  BUILTIN_FUNC("length", builtin::length, TYPE_Int, TYPE_String),

  BUILTIN_FUNC_FULL("push", builtin::push, true, true,
                    TypeInfo(TYPE_Vector, {TYPE_Template}), TYPE_None, {},
                    TYPE_Template),

  BUILTIN_FUNC_FULL("substr", builtin::substr, false, true, TYPE_String,
                    TYPE_String, TYPE_USize),

  BUILTIN_FUNC("input", builtin::input, TYPE_String),

  BUILTIN_FUNC("exit", builtin::exit, TYPE_Int),
};

std::vector<BuiltinFunc> const& BuiltinFunc::get_builtin_list()
{
  return ::_builtin_functions;
}
