#include "Utils.h"
#include "debug/alert.h"

#include "AST/AST.h"
#include "Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Sema.h"

#include "ASTWalker.h"

std::map<AST::Base*, TypeInfo> Sema::value_type_cache;

Sema::Sema(AST::Scope* root)
  : root(root),
    cur_impl(nullptr),
    impl_of(nullptr),
    CurScope(nullptr),
    CurFunc(nullptr)
{
}

Sema::~Sema()
{
}

Sema::ArgumentVector Sema::make_arg_vector(AST::CallFunc* ast)
{
  ArgumentVector ret;

  ret.caller = ast;

  for (auto&& arg : ast->args) {
    auto& W = ret.emplace_back(ArgumentWrap::ARG_Actual);

    W.typeinfo = this->check(arg);
    W.value = arg;
  }

  return ret;
}

Sema::ArgumentVector Sema::make_arg_vector(AST::Function* ast)
{
  ArgumentVector ret;

  ret.userdef_func = ast;

  for (auto&& arg : ast->args) {
    auto& W = ret.emplace_back(ArgumentWrap::ARG_Formal);

    W.typeinfo = this->check(arg->type);
    W.defined = arg->type;
  }

  return ret;
}

Sema::ArgumentVector Sema::make_arg_vector(BuiltinFunc const* func)
{
  ArgumentVector ret;

  ret.builtin = func;

  for (auto&& arg : func->arg_types) {
    auto& W = ret.emplace_back(ArgumentWrap::ARG_Formal);

    W.typeinfo = arg;
  }

  return ret;
}

Sema::ArgumentsComparationResult Sema::compare_argument(
  ArgumentVector const& formal, ArgumentVector const& actual)
{
  // formal = 定義側
  // actual = 呼び出し側

  // formal
  auto f_it = formal.begin();

  // actual
  auto a_it = actual.begin();

  while (f_it != formal.end()) {
    if (f_it->typeinfo.kind == TYPE_Args) {
      return ARG_OK;
    }

    if (a_it == actual.end()) {
      return ARG_Few;
      // Error(actual.caller, "too few arguments").emit().exit();
    }

    if (!f_it->typeinfo.equals(a_it->typeinfo)) {
      return ARG_Mismatch;
    }

    f_it++;
    a_it++;
  }

  if (a_it != actual.end()) {
    return ARG_Many;
  }

  return ARG_OK;
}

//
// ユーザー定義関数を探す
Sema::FunctionFindResult Sema::find_function(std::string_view name,
                                             bool have_self,
                                             std::optional<TypeInfo> self,
                                             ArgumentVector const& args)
{
  FunctionFindResult result;

  TypeInfo self_type;

  if (self) {
    self_type = self.value();
  }

  for (auto&& ctx : this->functions) {
    if (ctx.func->name.str != name)
      continue;

    if (ctx.is_have_self()) {
      if (!have_self || !ctx.self_type.value().equals(self_type))
        continue;
    }
    else if (have_self) {
      continue;
    }

    auto cmp = this->compare_argument(this->make_arg_vector(ctx.func), args);

    if (cmp == ARG_OK) {
      result.type = FunctionFindResult::FN_UserDefined;
      result.userdef = ctx.func;

      return result;
    }
  }

  // find builtin
  for (auto&& func : BuiltinFunc::get_builtin_list()) {
    if (func.name == name) {
      if (func.have_self != have_self)
        continue;

      if (have_self) {
        if (!func.self_type.equals(self_type))
          continue;
      }

      if (this->compare_argument(make_arg_vector(&func), args) != ARG_OK)
        continue;

      result.type = FunctionFindResult::FN_Builtin;
      result.builtin = &func;

      return result;
    }
  }

  return result;
}

//
// 今いる関数
AST::Function* Sema::get_cur_func()
{
  return this->CurFunc;
}

void Sema::expect_lvalue(AST::Base* _ast)
{
  switch (_ast->kind) {
    case AST_Variable:
      break;

    case AST_IndexRef: {
      astdef(IndexRef);

      this->expect_lvalue(ast->expr);

      break;
    }

    default:
      Error(_ast, "expected lvalue expression").emit().exit();
  }

  _ast->is_lvalue = true;
}

std::tuple<Sema::LocalVar*, size_t, size_t> Sema::find_variable(
  std::string_view const& name)
{
  for_indexed(step, scope, this->scope_list)
  {
    for_indexed(index, var, scope.lvar.variables)
    {
      if (var.name == name)
        return {&var, step, index};
    }
  }

  return {};
}

//
// キャプチャ追加
void Sema::begin_capture(Sema::CaptureFunction cap_func)
{
  this->captures.emplace_back(cap_func);
}

//
// キャプチャ削除
void Sema::end_capture()
{
  this->captures.pop_back();
}

void Sema::begin_return_capture(Sema::ReturnCaptureFunction cap_func)
{
  this->return_captures.emplace_back(cap_func);
}

void Sema::end_return_capture()
{
  this->return_captures.pop_back();
}

// int Sema::find_member(TypeInfo const& type, std::string_view name)
// {
//   for (int i = 0; auto&& m : ((AST::Struct*)type.userdef_type)->members) {
//     if (m.name == name)
//       return i;

//     i++;
//   }

//   return -1;
// }

Sema::SemaScope& Sema::get_cur_scope()
{
  return *this->scope_list.begin();
}

Sema::SemaScope& Sema::enter_scope(AST::Scope* ast)
{
  return this->scope_list.emplace_front(ast);
}

void Sema::leave_scope()
{
  this->scope_list.pop_front();
}

Sema::FunctionContext& Sema::add_function(AST::Function* func)
{
  auto& ctx = this->functions.emplace_back(func);

  ASTWalker::walk(func->code, [&ctx](AST::Base* ast) -> bool {
    if (ast->kind == AST_Return) {
      ctx.add_return_statement(ast);
    }

    return true;
  });

  return ctx;
}
