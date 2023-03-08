#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Sema.h"

#define astdef(T) auto ast = (AST::T*)_ast

/**
 * @brief 演算子に対する両辺の型が適切かどうかチェックする
 *
 * @param kind
 * @param lhs
 * @param rhs
 * @return std::optional<TypeInfo>
 */
std::optional<TypeInfo> Sema::is_valid_expr(AST::ExprKind kind,
                                            TypeInfo const& lhs,
                                            TypeInfo const& rhs)
{
  if (lhs.equals(TYPE_None) || rhs.equals(TYPE_None))
    return std::nullopt;

  switch (kind) {
    //
    // add
    case AST::EX_Add: {
      if (lhs.equals(rhs))
        return lhs;

      break;
    }

    //
    // sub
    case AST::EX_Sub: {
      // remove element from vector
      if (lhs.kind == TYPE_Vector) {
        if (lhs.type_params[0].equals(rhs)) {
          return lhs;
        }
      }

      // 数値同士
      if (lhs.is_numeric() && rhs.is_numeric()) {
        // float を優先する
        return lhs.kind == TYPE_Float ? lhs : rhs;
      }

      break;
    }

    //
    // mul
    case AST::EX_Mul: {
      if (rhs.is_numeric())
        return lhs;

      break;
    }

    //
    // div
    case AST::EX_Div: {
      if (lhs.is_numeric() && rhs.is_numeric())
        return lhs;
    }

    //
    // bit-calc
    case AST::EX_LShift:
    case AST::EX_RShift:
    case AST::EX_BitAND:
    case AST::EX_BitXOR:
    case AST::EX_BitOR:
      if (lhs.equals(TYPE_Int) && rhs.equals(TYPE_Int))
        return lhs;

      break;

    case AST::EX_And:
    case AST::EX_Or:
      if (lhs.equals(TYPE_Bool) && rhs.equals(TYPE_Bool))
        return lhs;

      break;

    default:
      todo_impl;
  }

  return std::nullopt;
}

// ------------------------------------------------ //
//  get_subscripted_type
// ------------------------------------------------ //
TypeInfo& Sema::get_subscripted_type(
    TypeInfo& type, std::vector<AST::Base*> const& indexes)
{
  auto* ret = &type;

  for (auto&& index : indexes) {
    auto index_type = this->check(index);

    switch (type.kind) {
      //
      // Vector
      case TYPE_Vector: {
        if (index_type.kind != TYPE_Int &&
            index_type.kind != TYPE_USize) {
          Error(index, "expected integer or usize").emit();
        }

        ret = &ret->type_params[0];
        break;
      }

      //
      // Disctionary
      case TYPE_Dict: {
        if (!index_type.equals(type.type_params[0])) {
          Error(index, "expecte '" +
                           type.type_params[0].to_string() +
                           "' but found '" +
                           index_type.to_string() + "'")
              .emit()
              .exit();
        }

        ret = &ret->type_params[1];
        break;
      }

      default:
        Error(index,
              "'" + type.to_string() + "' is not subscriptable")
            .emit()
            .exit();
    }
  }

  return *ret;
}

// ------------------------------------------------ //
//  check
// ------------------------------------------------ //
TypeInfo Sema::check(AST::Base* _ast)
{
  if (!_ast)
    return TYPE_None;

  for (auto&& cap : this->captures) {
    cap.func(_ast);
  }

  TypeInfo _ret = TYPE_None;

  switch (_ast->kind) {
    case AST_None:
      break;

    case AST_True:
    case AST_False:
      _ret = TYPE_Bool;
      break;

    case AST_UnaryMinus:
    case AST_UnaryPlus: {
      astdef(UnaryOp);

      _ret = this->check(ast->expr);

      if (!_ret.is_numeric())
        Error(ast->expr, "expected numeric").emit().exit();

      break;
    }

    case AST_Cast: {
      astdef(Cast);

      _ret = this->check(ast->cast_to);

      auto x = this->check(ast->expr);

      if (_ret.equals(x)) {
        Error(ast, "same type, don't need to use cast")
            .emit()
            .exit();
      }

      if (x.equals(TYPE_None)) {
        Error(ast,
              "cannot cast 'none' to '" + _ret.to_string() + "'")
            .emit()
            .exit();
      }

      switch (x.kind) {
        case TYPE_Int:
          switch (_ret.kind) {
            case TYPE_Float:
            case TYPE_Bool:
            case TYPE_Char:
              goto _cast_done;
          }
          break;

        case TYPE_Float:
          switch (_ret.kind) {
            case TYPE_Int:
            case TYPE_Bool:
              goto _cast_done;
          }
          break;

        case TYPE_Bool:
          switch (_ret.kind) {
            case TYPE_Int:
            case TYPE_Float:
              goto _cast_done;
          }
          break;
      }

      Error(ast, "cannot cast '" + _ret.to_string() + "' to '" +
                     x.to_string() + "'")
          .emit()
          .exit();

    _cast_done:
      break;
    }

    case AST_Break:
    case AST_Continue:
      break;

    case AST_Value: {
      TypeInfo ret;

      auto ast = (AST::Value*)_ast;

      switch (ast->token.kind) {
        case TOK_Int:
          ret = TYPE_Int;
          break;

        case TOK_USize:
          ret = TYPE_USize;
          break;

        case TOK_Float:
          ret = TYPE_Float;
          break;

        case TOK_Char:
          ret = TYPE_Char;
          break;

        case TOK_String:
          ret = TYPE_String;
          break;

        default:
          todo_impl;
      }

      _ret = ret;
      break;
    }

    case AST_Vector: {
      astdef(Vector);

      _ret = TYPE_Vector;

      for (auto&& e : ast->elements) {
        auto x = this->check(e);

        if (_ret.type_params.empty()) {
          _ret.type_params.emplace_back(x);
        }
        else if (!x.equals(_ret.type_params[0])) {
          Error(e, "type mismatch").emit().exit();
        }
      }

      break;
    }

    // 変数
    case AST_Variable: {
      _ret = this->check_as_left(_ast);
      break;
    }

    //
    // 関数呼び出し
    case AST_CallFunc: {
      _ret = this->check_function_call((AST::CallFunc*)_ast);
      break;
    }

    case AST_Dict: {
      astdef(Dict);

      alert;

      TypeInfo ret = TYPE_Dict;

      auto const key_type = ret.type_params.emplace_back(
          this->check(ast->key_type));

      auto const value_type = ret.type_params.emplace_back(
          this->check(ast->value_type));

      for (auto&& item : ast->elements) {
        if (auto x = this->check(item.key);
            !key_type.equals(x)) {
          Error(item.key, "expected '" + key_type.to_string() +
                              "' but found '" + x.to_string() +
                              "'")
              .emit();

          Error(ast->key_type, "specified here")
              .emit(Error::EL_Warning)
              .exit();
        }

        if (auto x = this->check(item.value);
            !value_type.equals(x)) {
          Error(item.value,
                "expected '" + value_type.to_string() +
                    "' but found '" + x.to_string() + "'")
              .emit();

          Error(ast->value_type, "specified here")
              .emit(Error::EL_Warning)
              .exit();
        }
      }

      // Sema::value_type_cache[_ast] = ret;

      _ret = ret;
      break;
    }

    case AST_IndexRef: {
      astdef(IndexRef);

      auto x = this->check(ast->expr);

      _ret = this->get_subscripted_type(x, ast->indexes);

      break;
    }

    case AST_Range: {
      astdef(Range);

      auto begin = this->check(ast->begin);
      auto end = this->check(ast->end);

      if (!begin.equals(end)) {
        Error(ast, "type mismatch").emit().exit();
      }

      if (!begin.equals(TYPE_Int)) {
        Error(ast, "expected integer").emit().exit();
      }

      return TYPE_Range;
    }

    //
    // 式
    case AST_Expr: {
      auto ast = (AST::Expr*)_ast;

      TypeInfo left = this->check(ast->first);

      for (auto&& elem : ast->elements) {
        auto right = this->check(elem.ast);

        if (auto res =
                this->is_valid_expr(elem.kind, left, right);
            !res) {
          Error(elem.op, "invalid operator").emit().exit();
        }
        else {
          left = res.value();
        }
      }

      _ret = left;
      break;
    }

    //
    // 代入式
    case AST_Assign: {
      astdef(Assign);

      auto dest = this->check_as_left(ast->dest);

      if (dest.is_const) {
        Error(ast, "destination is not mutable").emit().exit();
      }

      if (!dest.equals(this->check(ast->expr))) {
        Error(ast->token, "type mismatch").emit().exit();
      }

      _ret = dest;
      break;
    }

    //
    // 比較式
    case AST_Compare: {
      auto ast = (AST::Compare*)_ast;

      TypeInfo left = this->check(ast->first);

      for (auto&& elem : ast->elements) {
        auto right = this->check(elem.ast);

        if (!left.is_numeric() || !right.is_numeric()) {
          Error(elem.op, "invalid operator").emit().exit();
        }

        left = right;
      }

      _ret = TYPE_Bool;
      break;
    }

    //
    // 変数定義
    case AST_Let: {
      auto ast = (AST::VariableDeclaration*)_ast;

      auto& scope_emu = this->get_cur_scope();

      TypeInfo type;
      TypeInfo init_expr_type;

      if (ast->init) {
        init_expr_type = this->check(ast->init);
      }

      // 型が指定されてる
      if (ast->type) {
        type = this->check(ast->type);

        if (type.kind == TYPE_Vector) {
          if (init_expr_type.kind == TYPE_Vector &&
              init_expr_type.type_params.empty()) {
          }
        }

        // 初期化式がある場合
        //  =>
        //  指定された型と初期化式の型が一致しないならエラー
        if (ast->init && !type.equals(init_expr_type)) {
          Error(ast->init, "mismatched type").emit().exit();
        }
      }
      // 型が指定されてない
      else {
        // => 初期化式がないときエラー
        if (!ast->init) {
          Error(ast, "cannot deduction variable type")
              .emit()
              .exit();
        }

        type = std::move(init_expr_type);
      }

      // 同スコープ内で同じ名前の変数を探す
      auto pvar = scope_emu.find_var(ast->name);

      // すでに定義済みの同じ名前があって、違う型
      //  => シャドウイングする
      if (pvar && !pvar->type.equals(type)) {
        pvar->type = type;
      }
      // そうでなければ新規追加
      else {
        auto& var =
            scope_emu.variables.emplace_back(ast->name, type);

        var.index = scope_emu.variables.size() - 1;

        if (auto func = this->get_cur_func(); func) {
          func->var_count++;
        }
      }

      break;
    }

    //
    // return 文
    case AST_Return: {
      auto ast = (AST::Return*)_ast;

      auto cur_func = this->get_cur_func();

      // 関数の中ではない
      if (!cur_func) {
        Error(ast, "cannot use return-statement here")
            .emit()
            .exit();
      }
      // 式がない
      else if (!ast->expr) {
        break;
      }

      auto type = this->check(ast->expr);
      auto resp = cur_func->result_type;

      if (!type.equals(this->check(resp))) {
        Error(ast->expr, "type mismatch").emit();

        if (resp == nullptr) {
          Error(cur_func->code, "return type is not specified")
              .emit(Error::EL_Note);
        }
        else {
          Error(resp, "specified '" + resp->to_string() +
                          "' as result type, "
                          "but statement return '" +
                          type.to_string() + "'")
              .emit(Error::EL_Note);
        }
      }

      _ret = type;
      break;
    }

    //
    // if
    case AST_If: {
      auto ast = (AST::If*)_ast;

      if (!this->check(ast->condition).equals(TYPE_Bool)) {
        Error(ast->condition, "expected boolean expression")
            .emit()
            .exit();
      }

      auto xx = this->check(ast->if_true);

      if (!xx.equals(this->check(ast->if_false)))
        Error(ast, "type mismatch").emit().exit();

      _ret = xx;
      break;
    }

    //
    // for
    case AST_For: {
      astdef(For);

      auto& e =
          this->scope_list.emplace_front((AST::Scope*)ast->code);

      auto iterable = this->check(ast->iterable);

      if (!iterable.is_iterable()) {
        Error(ast->iterable, "expected iterable expression")
            .emit()
            .exit();
      }

      TypeInfo iter;

      switch (iterable.kind) {
        case TYPE_Range:
          iter = TYPE_Int;
          break;

        case TYPE_Vector:
        case TYPE_Dict:
          iter = iterable.type_params[0];
          break;
      }

      if (ast->iter->kind == AST_Variable) {
        e.variables.emplace_back(ast->iter->token.str, iter);
      }
      else if (auto x = this->check_as_left(ast->iter);
               !x.equals(iter)) {
        Error(ast->iter, "type mismatch").emit().exit();
      }

      this->check(ast->code);

      this->scope_list.pop_front();

      break;
    }

    //
    // スコープ
    case AST_Scope: {
      auto ast = (AST::Scope*)_ast;

      auto& e = this->scope_list.emplace_front(ast);

      auto voffs = this->variable_stack_offs;

      for (auto&& item : ast->list) {
        auto ww = this->check(item);
      }

      ast->var_count = e.variables.size();

      this->variable_stack_offs = voffs;

      this->scope_list.pop_front();
      break;
    }

    //
    // 関数
    case AST_Function: {
      auto ast = (AST::Function*)_ast;

      if (auto f = this->find_function(ast->name.str);
          f && f != ast) {
        Error(ast->name, "function '" +
                             std::string(ast->name.str) +
                             "' is already found")
            .emit()
            .exit();
      }

      this->function_history.emplace_front(ast);

      // 関数のスコープ　実装があるところ
      auto fn_scope = ast->code;

      // スコープ追加
      auto& S = this->scope_list.emplace_front(fn_scope);

      ast->var_count = ast->args.size();

      // 引数追加
      for (size_t ww = 0; auto&& x : ast->args) {
        auto& V = S.variables.emplace_back(x.name.str,
                                           this->check(x.type));

        V.index = ww++;
      }

      auto res_type = this->check(ast->result_type);

      std::vector<TypeInfo> return_types;

      this->begin_return_capture(
          [&](TypeInfo const& type, AST::Base* ast) {
            switch (ast->kind) {
              case AST_Return: {
                return_types.emplace_back(type);
                break;
              }
            }
          });

      // for (auto&& x : ast->code->list) {
      //   this->check(x);
      // }

      this->check(ast->code);

      if (!res_type.equals(TYPE_None) && return_types.empty()) {
        Error(ast,
              "return type is not none, "
              "but function return nothing")
            .emit();

        Error(ast->result_type, "return type specified here")
            .emit(Error::EL_Note)
            .exit();
      }

      this->end_return_capture();

      // スコープ削除
      this->scope_list.pop_front();

      this->function_history.pop_front();

      break;
    }

    //
    // 型
    case AST_Type: {
      static struct {
        TypeKind a;
        char const* b;
      } const names[]{
          {TYPE_None, "none"},     {TYPE_Int, "int"},
          {TYPE_USize, "usize"},   {TYPE_Float, "float"},
          {TYPE_Bool, "bool"},     {TYPE_Char, "char"},
          {TYPE_String, "string"}, {TYPE_Range, "range"},
          {TYPE_Vector, "vector"}, {TYPE_Dict, "dict"},
          {TYPE_Args, "args"},
      };

      auto ast = (AST::Type*)_ast;

      TypeInfo ret;

      for (auto&& x : names)
        if (ast->token.str == x.b) {
          ret = x.a;

          switch (x.a) {
            case TYPE_Range:
            case TYPE_Vector:
            case TYPE_Dict:
              if (ast->parameters.empty())
                Error(ast, "missing parameters").emit().exit();

              // todo: case user-def struct
          }

          goto skiperror009;
        }

      // todo: find userdef struct
      Error(ast, "unknown type name").emit().exit();

    skiperror009:;
      for (auto&& sub : ast->parameters) {
        ret.type_params.emplace_back(this->check(sub));
      }

      ret.is_const = ast->is_const;

      _ret = ret;
      break;
    }

    default:
      debug(printf("%d\n", _ast->kind));
      todo_impl;
  }

  value_type_cache[_ast] = _ret;

  for (auto&& retcap : this->return_captures) {
    retcap(_ret, _ast);
  }

  return _ret;
}

// ------------------------------------------------ //
//  check_as_left
// ------------------------------------------------ //
TypeInfo& Sema::check_as_left(AST::Base* _ast)
{
  switch (_ast->kind) {
    case AST_Variable: {
      astdef(Variable);

      for (size_t step = 0; auto&& S : this->scope_list) {
        for (auto it = S.variables.rbegin();
             it != S.variables.rend(); it++) {
          if (it->name == ast->token.str) {
            ast->step = step;
            ast->index = it->index;

            return it->type;
          }
        }

        step++;
      }

      Error(ast->token, "undefined variable name").emit().exit();
    }

    case AST_IndexRef: {
      astdef(IndexRef);

      return this->get_subscripted_type(
          this->check_as_left(ast->expr), ast->indexes);
    }
  }

  Error(_ast, "expected lvalue expression").emit().exit();
}

// ------------------------------------------------ //
//  check_function_call
// ------------------------------------------------ //
TypeInfo Sema::check_function_call(AST::CallFunc* ast)
{
  std::vector<TypeInfo> arg_types;

  // 引数
  for (auto&& arg : ast->args) {
    arg_types.emplace_back(this->check(arg));
  }

  // 同じ名前のビルトインを探す
  auto builtin_func_found = this->find_builtin_func(ast->name);

  if (builtin_func_found) {
    ast->is_builtin = true;
    ast->builtin_func = builtin_func_found;

    // 引数チェック
    auto formal = builtin_func_found->arg_types.begin();
    auto actual = arg_types.begin();

    auto e1 = builtin_func_found->arg_types.end();
    auto e2 = arg_types.end();

    auto arg = ast->args.begin();

    while (1) {
      // 呼び出し側の引数が終わった
      if (actual == e2) {
        if (formal == e1)  // 定義のほうも終わってたら抜ける
          break;

        // 定義側が引数リスト
        //  => 渡す引数なし
        if (formal->equals(TYPE_Args)) {
          break;
        }
      }
      // 定義側が終わった
      else if (formal == e1) {
        // 呼び出し側が多いのでエラー
        Error(*arg, "too many arguments").emit().exit();
      }
      // 定義側が引数リスト
      else if (formal->equals(TYPE_Args)) {
        break;
      }

      // 型が不一致の場合エラー
      if (!formal->equals(*actual)) {
        Error(*arg, "expected '" + formal->to_string() +
                        "' but found '" + actual->to_string() +
                        "'")
            .emit()
            .exit();
      }

      formal++;
      actual++;
      arg++;
    }

    return builtin_func_found->result_type;
  }

  // なければユーザー定義関数を探す
  if (auto func = this->find_function(ast->name); func) {
    ast->callee = func;

    // 仮引数 無し
    // if( func->args.empty() ) {
    if (0) {
      // ここのスコープ使わない
      if (!ast->args.empty()) {
        Error(ast, "too many arguments").emit().exit();
      }
    }
    // 仮引数 有り
    else {
      /// こっちに入る

      // 定義側の引数
      auto formal_arg_it = func->args.begin();

      // 呼び出す方の引数
      auto act_arg_it = ast->args.begin();

      while (1) {
        if (auto Q = formal_arg_it == func->args.end();
            Q != (act_arg_it == ast->args.end())) {
          if (Q) {  // 定義側の引数
            Error(*act_arg_it, "too many arguments")
                .emit()
                .exit();
          }

          Error(ast, "too few arguments").emit().exit();
        }
        else if (Q) {  // true!=true で、ループ終了
          break;
        }

        auto arg = *act_arg_it;

        auto aa = this->check(formal_arg_it->type);
        auto bb = this->check(*act_arg_it);

        if (!aa.equals(bb)) {
          Error(arg, "mismatched type").emit();
        }

        formal_arg_it++;
        act_arg_it++;
      }

    }  // 引数チェック終了

    return this->check(func->result_type);
  }

  Error(ast, "undefined function name").emit().exit();
}
