#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Sema.h"

#define astdef(T) auto ast = (AST::T*)_ast

std::map<AST::Base*, TypeInfo> Sema::value_type_cache;

//
// キャッシュを作成しても問題ない AST かどうかを判別する
static bool is_cache_allowed(ASTKind kind)
{
  switch (kind) {
    case AST_Value:
      return true;
  }

  return false;
}

Sema::Sema(AST::Scope* root)
  : root(root)
{
}

Sema::~Sema()
{
}

Sema::ArgVector Sema::ArgumentWrap::make_vector_from_call(Sema& S,
                                                          AST::CallFunc* ast)
{
  ArgVector ret;

  ret.caller = ast;

  for (auto&& arg : ast->args) {
    auto& W = ret.emplace_back(ArgumentWrap::ARG_Actual);

    W.typeinfo = S.check(arg);
    W.value = arg;
  }

  return ret;
}

Sema::ArgVector Sema::ArgumentWrap::make_vector_from_function(
  Sema& S, AST::Function* ast)
{
  ArgVector ret;

  ret.userdef_func = ast;

  for (auto&& arg : ast->args) {
    auto& W = ret.emplace_back(ArgumentWrap::ARG_Formal);

    W.typeinfo = S.check(arg->type);
    W.defined = arg->type;
  }

  return ret;
}

Sema::ArgVector Sema::ArgumentWrap::make_vector_from_builtin(
  Sema& S, BuiltinFunc const* func)
{
  ArgVector ret;

  ret.builtin = func;

  for (auto&& arg : func->arg_types) {
    auto& W = ret.emplace_back(ArgumentWrap::ARG_Formal);

    W.typeinfo = arg;
  }

  return ret;
}

void Sema::do_check()
{
  TypeRecursionDetector tr{*this};

  for (auto&& x : this->root->list) {
    switch (x->kind) {
      case AST_Struct:
        tr.walk((AST::Typeable*)x);
        break;
    }
  }

  this->check(this->root);
}

Sema::ArgumentsComparationResult Sema::compare_argument(ArgVector const& formal,
                                                        ArgVector const& actual)
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
      // Error(ERR_TypeMismatch, a_it->get_ast(),
      //       "expected '" + f_it->typeinfo.to_string() + "' but found '" +
      //         a_it->typeinfo.to_string() + "'")
      //   .emit()
      //   .exit();
    }

    f_it++;
    a_it++;
  }

  if (a_it != actual.end()) {
    return ARG_Many;
    // Error(actual.caller, "too many arguments").emit().exit();
  }

  return ARG_OK;
}

//
// 名前から型を探す
std::optional<TypeInfo> Sema::get_type_from_name(std::string_view name)
{
  if (auto builtin = TypeInfo::get_kind_from_name(name); builtin)
    return builtin.value();

  if (auto usertype = this->find_usertype(name); usertype) {
    TypeInfo ret{TYPE_UserDef};

    ret.userdef_type = usertype;

    if (usertype->kind == AST_Struct) {
      for (auto&& member : ((AST::Struct*)usertype)->members)
        ret.members.emplace_back(member.name, this->check(member.type));
    }

    return ret;
  }

  return std::nullopt;
}

//
// ユーザー定義関数を探す
Sema::FunctionFindResult Sema::find_function(std::string_view name,
                                             bool have_self,
                                             TypeInfo const& self_type,
                                             ArgVector const& args)
{
  FunctionFindResult result;

  auto const& builtins = BuiltinFunc::get_builtin_list();

  // find builtin
  for (auto&& func : builtins) {
    if (func.name == name) {
      if (func.have_self != have_self)
        continue;

      if (!func.self_type.equals(self_type))
        continue;

      if (this->compare_argument(
            ArgumentWrap::make_vector_from_builtin(*this, &func), args) !=
          ARG_OK)
        continue;

      result.type = FunctionFindResult::FN_Builtin;
      result.builtin = &func;

      return result;
    }
  }

  // find user-def
  for (auto&& item : this->root->list) {
    auto func = (AST::Function*)item;

    if (item->kind == AST_Function && func->name.str == name) {
      if (func->have_self != have_self)
        continue;

      if (!this->check(func->self_type).equals(self_type))
        continue;

      if (this->compare_argument(
            ArgumentWrap::make_vector_from_function(*this, func), args) !=
          ARG_OK)
        continue;

      result.type = FunctionFindResult::FN_UserDefined;
      result.userdef = func;

      return result;
    }
  }

  return result;
}

//
// ユーザー定義構造体を探す
AST::Typeable* Sema::find_usertype(std::string_view name)
{
  for (auto&& item : this->root->list) {
    switch (item->kind) {
      case AST_Enum:
      case AST_Struct:
        if (((AST::Typeable*)item)->name == name)
          return (AST::Typeable*)item;
    }
  }

  return nullptr;
}

//
// 組み込み関数を探す
// BuiltinFunc const* Sema::find_builtin_func(std::string_view name)
// {
//   for (auto&& builtinfunc : BuiltinFunc::get_builtin_list())
//     if (builtinfunc.name == name)
//       return &builtinfunc;

//   return nullptr;
// }

//
// 今いる関数
AST::Function* Sema::get_cur_func()
{
  return *this->function_history.begin();
}

// ----------
//  Expect value to ast
// ----------------
TypeInfo Sema::expect(TypeInfo const& expected, AST::Base* ast)
{
  auto type = this->check(ast);

  // 同じならそのまま返す
  if (expected.equals(type))
    return expected;

  switch (expected.kind) {
    case TYPE_Vector: {
      if (ast->is_empty_vector())
        goto matched;

      break;
    }

    case TYPE_Dict: {
      auto x = (AST::Dict*)ast;

      if (ast->is_empty_scope())
        goto matched;

      if (ast->kind == AST_Dict && !x->key_type && x->elements.empty())
        goto matched;

      break;
    }
  }

  //
  // ast がスコープ
  if (ast->kind == AST_Scope) {
    // 最後の要素を評価結果とする場合、エラー指摘場所をそれに変更する
    if (auto x = (AST::Scope*)ast; x->return_last_expr)
      ast = *x->list.rbegin();
  }

  Error(ast, "expected '" + expected.to_string() + "' but found '" +
               type.to_string() + "'")
    .emit()
    .exit();

matched:
  return this->value_type_cache[ast] = expected;
}

void Sema::TypeRecursionDetector::walk(AST::Typeable* ast)
{
  switch (ast->kind) {
    case AST_Type: {
      if (ast->name == "vector") {
        return;
      }

      break;
    }
  }

  if (std::find(this->stack.begin(), this->stack.end(), ast) !=
      this->stack.end()) {
    Error(ast,
          "recursive type '" + std::string(ast->name) + "' have infinity size")
      .single_line()
      .emit();

    Error(*this->stack.rbegin(), "recursive without indirection")
      .emit(EL_Note)
      .exit();
  }

  this->stack.emplace_back(ast);

  switch (ast->kind) {
    case AST_Enum: {
      auto x = (AST::Enum*)ast;

      for (auto&& e : x->enumerators) {
        if (e.value_type)
          this->walk(e.value_type);
      }

      break;
    }

    case AST_Struct: {
      auto x = (AST::Struct*)ast;

      for (auto&& m : x->members)
        this->walk(m.type);

      break;
    }

    case AST_Type: {
      auto x = (AST::Type*)ast;

      if (auto find = this->S.find_usertype(x->name); find) {
        this->walk(find);
      }

      break;
    }
  }

  this->stack.pop_back();
}

//
// 演算子の型の組み合わせが正しいかチェックする
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

      break;
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
TypeInfo Sema::check_indexref(AST::IndexRef* ast)
{
  TypeInfo type;

  alert;

  if (AST::Typeable* usr = nullptr;
      ast->expr->kind == AST_Variable &&
      (usr = this->find_usertype(((AST::Variable*)ast->expr)->name))) {
    type.kind = TYPE_UserDef;
    type.userdef_type = usr;

    alert;

    switch (usr->kind) {
      case AST_Enum: {
        auto ast_enum = (AST::Enum*)usr;

        ast->is_enum = true;
        ast->enum_type = ast_enum;

        if (ast->indexes.empty()) {
          Error(ERR_InvalidSyntax, ast->expr,
                "expected enumerator name after this token")
            .emit()
            .exit();
        }
        else if (ast->indexes.size() > 1) {
          Error(ERR_InvalidSyntax, ast->indexes[1].ast, "???").emit().exit();
        }

        auto& sub = ast->indexes[0].ast;

        std::string_view name;
        AST::Base* init = nullptr;
        TypeInfo init_type;

        switch (sub->kind) {
          case AST_Variable:
            name = ((AST::Variable*)sub)->name;
            break;

          case AST_CallFunc: {
            auto x = (AST::CallFunc*)sub;

            name = x->name;

            if (!x->args.empty()) {
              init = x->args[0];
              init_type = this->check(init);
            }

            break;
          }
        }

        for (auto&& E : ast_enum->enumerators) {
          if (E.name == name) {
            if (E.value_type) {
              auto deftype = this->check(E.value_type);

              if (!init) {
                Error(ERR_InvalidSyntax, sub,
                      "expected initializer of '" + deftype.to_string() +
                        "' after this token")
                  .emit()
                  .exit();
              }
              else if (!deftype.equals(init_type)) {
                Error(ERR_TypeMismatch, init,
                      "expected '" + deftype.to_string() + "' but found '" +
                        init_type.to_string() + "'")
                  .emit()
                  .exit();
              }
            }
            else if (init) {
              Error(ERR_InvalidSyntax, sub->token, "dont need initializer")
                .emit()
                .exit();
            }

            return TYPE_Enumerator;
          }

          ast->enumerator_index++;
        }

        Error(ERR_Undefined, sub,
              "enum '" + std::string(ast_enum->name) +
                "' don't have a enumerator '" + std::string(name) + "'")
          .emit()
          .exit();

        break;
      }

      case AST_Impl:
        break;
    }

    todo_impl;
  }

  alert;
  type = this->check(ast->expr);

  for (auto&& index : ast->indexes) {
    switch (index.kind) {
      //
      // 配列添字
      case AST::IndexRef::Subscript::SUB_Index: {
        auto index_type = this->check(index.ast);

        switch (type.kind) {
          //
          // vector or string
          case TYPE_String:
          case TYPE_Vector:
            if (index_type.kind != TYPE_Int && index_type.kind != TYPE_USize) {
              Error(index.ast, "expected integer or usize").emit();
            }

            if (type.kind == TYPE_String)
              type = TYPE_Char;
            else
              type = type.type_params[0];

            break;

          //
          // Disctionary
          case TYPE_Dict: {
            // キーの型と一致しない場合エラー
            if (!index_type.equals(type.type_params[0])) {
              Error(index.ast, "expecte '" + type.type_params[0].to_string() +
                                 "' but found '" + index_type.to_string() + "'")
                .emit()
                .exit();
            }

            // value
            type = type.type_params[1];
            break;
          }

          default:
            Error(index.ast, "'" + type.to_string() + "' is not subscriptable")
              .emit()
              .exit();

            break;
        }

        break;
      }

      //
      // メンバアクセス
      case AST::IndexRef::Subscript::SUB_Member: {
        // if (type.kind == TYPE_Enumerator) {
        //   if (index.ast->kind == AST_Variable &&
        //       ((AST::Variable*)index.ast)->name == "value") {
        //     type = this->check(((AST::Enum*)type.userdef_type)
        //                          ->enumerators[ast->enumerator_index]
        //                          .value_type);

        //     break;
        //   }
        // }

        if (type.kind != TYPE_UserDef) {
          Error(index.ast,
                "'" + type.to_string() + "' type object don't have any members")
            .emit()
            .exit();
        }

        std::string name;
        std::vector<AST::Type*>* p_params = nullptr;
        AST::Base* init = nullptr;
        TypeInfo init_type;

        switch (index.ast->kind) {
          case AST_Variable:
            name = ((AST::Variable*)index.ast)->name;
            break;

          case AST_CallFunc: {
            auto cf = (AST::CallFunc*)index.ast;

            name = cf->name;

            break;
          }

          default:
            Error(ERR_InvalidSyntax, index.ast, "invalid syntax").emit().exit();
        }

        switch (type.userdef_type->kind) {
          //
          // 構造体
          case AST_Struct: {
            // AST 構造体へのポインタ
            auto pStruct = (AST::Struct*)type.userdef_type;

            // 名前が一致するメンバを探す
            for (auto&& M : pStruct->members) {
              // 一致する名前を発見
              //  --> ループ抜ける (--> @found_member)
              if (M.name == name) {
                type = this->check(M.type);
                goto found_member;
              }

              ((AST::Variable*)index.ast)->index++;
            }

            // 一致するものがない
            Error(ERR_Undefined, index.ast,
                  "struct '" + std::string(pStruct->name) +
                    "' don't have a member '" + name + "'")
              .emit()
              .exit();

          found_member:
            break;
          }

          default:
            todo_impl;
        }

        break;
      }

      default:
        todo_impl;
    }
  }

  return type;
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

  if (is_cache_allowed(_ast->kind)) {
    if (this->value_type_cache.contains(_ast))
      return this->value_type_cache[_ast];
  }

  auto& _ret = this->value_type_cache[_ast];

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
        Error(ast, "same type, don't need to use cast").emit().exit();
      }

      if (x.equals(TYPE_None)) {
        Error(ast, "cannot cast 'none' to '" + _ret.to_string() + "'")
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

      Error(ast,
            "cannot cast '" + _ret.to_string() + "' to '" + x.to_string() + "'")
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
      astdef(Variable);

      if (auto [p, s, i] = this->find_variable(ast->name); p) {
        _ret = p->type;

        ast->step = s;
        ast->index = i;
      }
      else {
        Error(ERR_Undefined, ast, "undefined variable name").emit().exit();
      }

      break;
    }

    //
    // 関数呼び出し
    case AST_CallFunc: {
      _ret = this->check_function_call((AST::CallFunc*)_ast, false, {});
      break;
    }

    //
    // Type Constructor
    case AST_TypeConstructor: {
      astdef(TypeConstructor);

      auto& pStruct = ast->p_struct;

      auto& type = _ret;
      type = this->check(ast->type);

      if (type.kind != TYPE_UserDef || type.userdef_type->kind != AST_Struct) {
        Error(ast->type, "aiueo@@@").emit().exit();
      }

      pStruct = (AST::Struct*)type.userdef_type;

      //
      // すべてのメンバを初期化するのが前提なので
      // 個数、名前は完全一致していなければエラー
      //

      // 初期化子の数が合わない
      //  => エラー
      if (ast->init_pair_list.size() != pStruct->members.size()) {
        Error(ERR_InvalidInitializer, ast, "don't matching member size")
          .emit()
          .exit();
      }

      //
      // 要素を全部チェック
      for (size_t ast_member_index = 0; auto&& pair : ast->init_pair_list) {
        //
        // メンバ
        auto const& ast_member = pStruct->members[ast_member_index];

        // 名前
        if (pair.name != ast_member.name) {
          Error(*pair.t_name, "no match member name").emit().exit();
        }

        // 型
        this->expect(this->check(ast_member.type), pair.expr);

        ast_member_index++;
      }

      break;

      _ret = type;
      break;
    }

    case AST_Dict: {
      astdef(Dict);

      _ret = TYPE_Dict;

      TypeInfo key_type;
      TypeInfo value_type;

      auto item_iter = ast->elements.begin();

      if (ast->key_type) {
        _ret.type_params = {this->check(ast->key_type),
                            this->check(ast->value_type)};
      }
      else {
        _ret.type_params = {this->check(item_iter->key),
                            this->check(item_iter->value)};

        item_iter++;
      }

      for (; item_iter != ast->elements.end(); item_iter++) {
        this->expect(key_type, item_iter->key);
        this->expect(value_type, item_iter->value);
      }

      break;
    }

    //
    // 配列添字・メンバアクセス
    case AST_IndexRef: {
      _ret = this->check_indexref((AST::IndexRef*)_ast);
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

      _ret = TYPE_Range;
      break;
    }

    //
    // 式
    case AST_Expr: {
      auto ast = (AST::Expr*)_ast;

      TypeInfo left = this->check(ast->first);

      for (auto&& elem : ast->elements) {
        auto right = this->check(elem.ast);

        if (auto res = this->is_valid_expr(elem.kind, left, right); !res) {
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

      auto dest = this->as_lvalue(ast->dest);

      if (dest.is_const) {
        Error(ast, "destination is not mutable").emit().exit();
      }

      this->expect(dest, ast->expr);

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

        if (elem.kind == AST::CMP_Equal || elem.kind == AST::CMP_NotEqual) {
          if (!left.equals(right))
            Error(elem.op, "type mismatch").emit().exit();
        }
        else if (!left.is_numeric() || !right.is_numeric()) {
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

      // 型が指定されてる
      if (ast->type) {
        type = this->check(ast->type);

        auto tmp = this->check(ast->init);

        if (!tmp.equals(type))
          ast->ignore_initializer = true;

        type = this->expect(type, ast->init);
      }
      // 型が指定されてない
      else {
        // => 初期化式がないときエラー
        if (!ast->init) {
          Error(ast, "cannot deduction variable type").emit().exit();
        }

        type = this->check(ast->init);
      }

      // 同スコープ内で同じ名前の変数を探す
      auto pvar = scope_emu.lvar.find_var(ast->name);

      // すでに定義済みの同じ名前があって、違う型
      //  => シャドウイングする
      if (pvar && !pvar->type.equals(type)) {
        pvar->type = type;

        ast->is_shadowing = true;
        // ast->index = pvar->index;
      }
      // そうでなければ新規追加
      else {
        auto& var = scope_emu.lvar.append(type, ast->name);

        // ast->index = var.index = scope_emu.lvar.variables.size() - 1;
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
        Error(ast, "cannot use return-statement here").emit().exit();
      }

      if (ast->expr) {
        _ret = this->expect(this->check(cur_func->result_type), ast->expr);
      }
      else if (auto t = this->check(cur_func->result_type);
               !t.equals(TYPE_None)) {
        Error(ast, "expected '" + t.to_string() +
                     "' type expression after this token")
          .emit()
          .exit();
      }

      break;
    }

    //
    // if
    case AST_If: {
      auto ast = (AST::If*)_ast;

      if (!this->check(ast->condition).equals(TYPE_Bool)) {
        Error(ast->condition, "expected boolean expression").emit().exit();
      }

      auto xx = this->check(ast->if_true);

      if (ast->if_false)
        return this->expect(xx, ast->if_false);

      break;
    }

    //
    // switch
    case AST_Switch: {
      astdef(Switch);

      auto item = this->check(ast->expr);

      bool detected = false;
      TypeInfo type;

      for (auto&& case_ast : ast->cases) {
        auto x = this->check(case_ast->cond);

        if (!x.equals(item) && !x.equals(TYPE_Bool)) {
          Error(case_ast->cond, "expected boolean or '" + item.to_string() +
                                  "', but found '" + x.to_string() + "'")
            .emit()
            .exit();
        }

        auto tmp = this->check(case_ast->scope);

        if (detected && !type.equals(tmp)) {
          if (type.equals(TYPE_None)) {
            Error(ERR_TypeMismatch, *case_ast->scope->end_token,
                  "expected semicolon before this token")
              .emit()
              .exit();
          }

          Error(
            ERR_TypeMismatch, *case_ast->scope->end_token,
            "expected '" + type.to_string() + "' expression before this token")
            .emit()
            .exit();
        }
        else {
          type = tmp;
          detected = true;
        }
      }

      break;
    }

    //
    // loop
    case AST_Loop: {
      astdef(Loop);

      this->check(ast->code);

      break;
    }

    //
    // for
    case AST_For: {
      astdef(For);

      auto& e = this->enter_scope((AST::Scope*)ast->code);

      auto iterable = this->check(ast->iterable);

      TypeInfo iter;

      switch (iterable.kind) {
        case TYPE_Range:
          iter = TYPE_Int;
          break;

        case TYPE_Vector:
        case TYPE_Dict:
          iter = iterable.type_params[0];
          break;

        default:
          Error(ast->iterable, "expected iterable expression").emit().exit();
      }

      //
      // イテレータが変数だったら自動で定義
      if (ast->iter->kind == AST_Variable) {
        alertmsg(ast->iter->token.str);

        e.lvar.append(iter, ast->iter->token.str);
      }

      //  --> 左辺値でない あるいは 型が合わないならエラー
      if (!this->as_lvalue(ast->iter).equals(iter)) {
        Error(ast->iter, "type mismatch").emit().exit();
      }

      this->check(ast->code);

      this->leave_scope();

      break;
    }

    //
    // while
    case AST_While: {
      astdef(While);

      this->expect(TYPE_Bool, ast->cond);
      this->check(ast->code);

      break;
    }

    //
    // do-while
    case AST_DoWhile: {
      astdef(DoWhile);

      this->check(ast->code);
      this->expect(TYPE_Bool, ast->cond);

      break;
    }

    //
    // スコープ
    case AST_Scope: {
      auto ast = (AST::Scope*)_ast;

      // empty scope
      if (ast->list.empty())
        break;

      this->enter_scope(ast);

      if (ast->return_last_expr) {
        auto it = ast->list.begin();

        while (*it != *ast->list.rbegin())
          this->check(*it++);

        _ret = this->check(*it);
      }
      else {
        for (auto&& e : ast->list)
          this->check(e);
      }

      this->leave_scope();

      break;
    }

    //
    // 関数
    case AST_Function: {
      auto ast = (AST::Function*)_ast;

      if (auto f = this->find_function(
            ast->name.str, ast->have_self, this->check(ast->self_type),
            ArgumentWrap::make_vector_from_function(*this, ast));
          f.found() && f.userdef != ast) {
        Error(ast->name,
              "function '" + std::string(ast->name.str) + "' is already found")
          .emit()
          .exit();
      }

      this->function_history.emplace_front(ast);

      // 関数のスコープ　実装があるところ
      auto fn_scope = ast->code;

      // スコープ追加
      auto& S = this->enter_scope(fn_scope);

      // 引数追加
      for (auto&& arg : ast->args) {
        S.lvar.append(this->check(arg->type), arg->name);
      }

      auto res_type = this->check(ast->result_type);

      std::vector<TypeInfo> return_types;

      this->begin_return_capture([&](TypeInfo const& type, AST::Base* ast) {
        switch (ast->kind) {
          case AST_Return: {
            return_types.emplace_back(type);
            break;
          }
        }
      });

      auto code_type = this->check(ast->code);

      if (ast->code->return_last_expr) {
        if (!code_type.equals(res_type)) {
          Error(ERR_TypeMismatch, (*ast->code->list.rbegin())->token,
                "expected '" + res_type.to_string() + "' but found '" +
                  code_type.to_string() + "'")
            .emit();

          goto err_specify_ast;
        }
      }
      else if (!res_type.equals(TYPE_None)) {
        if (ast->code->list.empty() || return_types.empty()) {
          Error(ast->token,
                "return type is not none, "
                "but function return nothing")
            .emit();

        err_specify_ast:
          Error(ast->result_type ? ast->result_type->token : ast->token,
                std::string("return type ") +
                  (ast->result_type ? "" : "implicitly ") + "specified with '" +
                  res_type.to_string() + "' here")
            .emit(EL_Note)
            .exit();
        }

        auto last = *ast->code->list.rbegin();

        auto semi = last->end_token;
        semi++;

        if (last->kind != AST_Return) {
          Error(*semi, "expected '" + res_type.to_string() +
                         "' type expression after this token")
            .emit();

          Error(last,
                "semicolon ignores the evaluated result of "
                "this")
            .emit(EL_Note)
            .exit();
        }
      }

      this->end_return_capture();

      // スコープ削除
      this->leave_scope();

      this->function_history.pop_front();

      break;
    }

    case AST_Enum: {
      astdef(Enum);

      std::map<std::string_view, bool> map;

      for (auto&& e : ast->enumerators) {
        if (!(map[e.name] ^= 1)) {
          Error(ERR_MultipleDefined, e.token, "multiple definition")
            .emit()
            .exit();
        }

        this->check(e.value_type);
      }

      break;
    }

    // struct
    case AST_Struct: {
      astdef(Struct);

      std::map<std::string_view, bool> map;

      for (auto&& item : ast->members) {
        if (!(map[item.name] ^= 1)) {
          Error(ERR_MultipleDefined, item.token, "multiple definition")
            .emit()
            .exit();
        }

        this->check(item.type);
      }

      _ret = TYPE_UserDef;
      _ret.userdef_type = ast;

      break;
    }

    case AST_Impl: {
      astdef(Impl);

      todo_impl;

      break;
    }

    //
    // 型
    case AST_Type: {
      auto ast = (AST::Type*)_ast;

      auto& ret = _ret;

      this->type_check_stack.emplace_back(ast);

      if (auto res = this->get_type_from_name(ast->name); res) {
        ret = res.value();
      }
      else {
        Error(ast, "undefined type name '" + std::string(ast->name) + "'")
          .emit()
          .exit();
      }

      switch (ret.kind) {
        case TYPE_UserDef:
          // todo:
          // if struct->params is zero: break
          break;

        case TYPE_Vector:
        case TYPE_Dict:
          if (ast->parameters.empty())
            Error(ast, "missing parameters").emit().exit();

          break;
      }

      //
      // add parameters
      for (auto&& sub : ast->parameters) {
        ret.type_params.emplace_back(this->check(sub));
      }

      //
      // is_const
      ret.is_const = ast->is_const;

      this->type_check_stack.pop_back();

      _ret = ret;
      break;
    }

    default:
      debug(printf("%d\n", _ast->kind));
      todo_impl;
  }

  for (auto&& retcap : this->return_captures) {
    retcap(_ret, _ast);
  }

#if METRO_DEBUG
  _ast->__checked = true;

#endif

  return _ret;
}

// ------------------------------------------------ //
//  check_as_left
// ------------------------------------------------ //
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
}

TypeInfo Sema::as_lvalue(AST::Base* ast)
{
  this->expect_lvalue(ast);
  return this->check(ast);
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

// ------------------------------------------------ //
//  関数呼び出しをチェックする
// ------------------------------------------------ //
TypeInfo Sema::check_function_call(AST::CallFunc* ast, bool have_self,
                                   TypeInfo const& self_type)
{
  using FFResult = FunctionFindResult;

  //
  // 引数
  auto args = ArgumentWrap::make_vector_from_call(*this, ast);

  // 同じ名前の関数を探す
  auto result = this->find_function(ast->name, have_self, self_type, args);

  //
  // ビルトイン
  if (result.type == FFResult::FN_Builtin) {
    ast->is_builtin = true;
    ast->builtin_func = result.builtin;

    // this->compare_argument(
    //   ArgumentWrap::make_vector_from_builtin(*this, result.builtin), args);

    return result.builtin->result_type;
  }

  // ユーザー定義関数
  if (result.type == FFResult::FN_UserDefined) {
    ast->callee = result.userdef;

    // this->compare_argument(
    //   ArgumentWrap::make_vector_from_function(*this, result.userdef), args);

    return this->check(result.userdef->result_type);
  }

  auto func_name = std::string(ast->name) + "(" +
                   Utils::String::join("", args,
                                       [](auto& aw) {
                                         return aw.typeinfo.to_string();
                                       }) +
                   ")";

  Error(ast, "function '" + func_name + "' not found").emit().exit();
}

void Sema::check_struct(AST::Struct* ast)
{
  std::map<std::string_view, bool> map;

  for (auto&& item : ast->members) {
    if (!(map[item.name] ^= 1)) {
      Error(ERR_MultipleDefined, item.token, "multiple definition")
        .emit()
        .exit();
    }

    this->check(item.type);
  }
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
