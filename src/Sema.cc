#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Sema.h"

#define astdef(T) auto ast = (AST::T*)_ast

std::map<AST::Base*, TypeInfo> Sema::value_type_cache;

Sema::Sema(AST::Scope* root)
  : root(root)
{
}

Sema::~Sema()
{
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
AST::Function* Sema::find_function(std::string_view name)
{
  for (auto&& item : this->root->list)
    if (item->kind == AST_Function && ((AST::Function*)item)->name.str == name)
      return (AST::Function*)item;

  return nullptr;
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
BuiltinFunc const* Sema::find_builtin_func(std::string_view name)
{
  for (auto&& builtinfunc : BuiltinFunc::get_builtin_list())
    if (builtinfunc.name == name)
      return &builtinfunc;

  return nullptr;
}

//
// 今いる関数
AST::Function* Sema::get_cur_func()
{
  return *this->function_history.begin();
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

TypeInfo Sema::expect(TypeInfo const& expected, AST::Base* ast)
{
  auto type = this->check(ast);

  if (type.equals(expected))
    return expected;

  switch (type.kind) {
    case TYPE_Vector: {
      auto x = (AST::Vector*)ast;

      if (x->elements.empty())
        return expected;

      break;
    }

    case TYPE_Dict: {
      if (ast->kind == AST_Scope && ((AST::Scope*)ast)->list.empty())
        return expected;

      auto x = (AST::Dict*)ast;

      if (!!x->key_type && x->elements.empty())
        return expected;

      break;
    }
  }

  if (ast->kind == AST_Scope) {
    if (auto x = (AST::Scope*)ast; x->return_last_expr)
      ast = *x->list.rbegin();
  }

  Error(ast, "expected '" + expected.to_string() + "' but found '" +
               type.to_string() + "'")
    .emit()
    .exit();
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

  if (AST::Typeable * usr;
      ast->expr->kind == AST_Variable &&
      (usr = this->find_usertype(((AST::Variable*)ast->expr)->name)) &&
      usr->kind == AST_Enum) {
    type.kind = TYPE_UserDef;
    type.userdef_type = usr;

    ast->is_enum = true;
    ast->enum_type = (AST::Enum*)usr;
  }
  else {
    type = this->check(ast->expr);
  }

  for (auto&& index : ast->indexes) {
    switch (index.kind) {
      //
      // 配列添字
      case AST::IndexRef::Subscript::SUB_Index: {
        auto index_type = this->check(index.ast);

        switch (type.kind) {
          //
          // Vector
          case TYPE_Vector: {
            if (index_type.kind != TYPE_Int && index_type.kind != TYPE_USize) {
              Error(index.ast, "expected integer or usize").emit();
            }

            type = type.type_params[0];
            break;
          }

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
        if (type.kind != TYPE_UserDef) {
          Error(index.ast,
                "'" + type.to_string() + "' type object don't have any members")
            .emit()
            .exit();
        }

        switch (index.ast->kind) {
          // 識別子
          case AST_Variable: {
            auto var = (AST::Variable*)index.ast;

            switch (type.userdef_type->kind) {
              //
              // 列挙型
              case AST_Enum: {
                auto pEnum = (AST::Enum*)type.userdef_type;

                // 名前が一致する列挙値を探す
                for (auto&& E : pEnum->enumerators) {
                  // 一致する名前を見つけた
                  // --> ループ抜ける (--> @found_enumerator)
                  if (E.name == var->name) {
                    type.kind = TYPE_Enumerator;
                    assert(type.userdef_type);

                    goto found_enumerator;
                  }

                  //
                  ast->enum_value++;
                }

                // 一致するものがない
                Error(ERR_Undefined, index.ast,
                      "enum '" + std::string(pEnum->name) +
                        "' don't have a enumerator '" + std::string(var->name) +
                        "'")
                  .emit()
                  .exit();

              found_enumerator:
                break;
              }

              //
              // 構造体
              case AST_Struct: {
                // AST 構造体へのポインタ
                auto pStruct = (AST::Struct*)type.userdef_type;

                // 名前が一致するメンバを探す
                for (auto&& M : pStruct->members) {
                  // 一致する名前を発見
                  //  --> ループ抜ける (--> @found_member)
                  if (M.name == var->name) {
                    type = this->check(M.type);
                    goto found_member;
                  }

                  var->index++;
                }

                // 一致するものがない
                Error(ERR_Undefined, index.ast,
                      "struct '" + std::string(pStruct->name) +
                        "' don't have a member '" + std::string(var->name) +
                        "'")
                  .emit()
                  .exit();

              found_member:
                break;
              }
            }

            break;
          }

          default:
            alertmsg((int)index.ast->kind);
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

  if (this->value_type_cache.contains(_ast)) {
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
      _ret = this->check_as_left(_ast);
      break;
    }

    //
    // 関数呼び出し
    case AST_CallFunc: {
      _ret = this->check_function_call((AST::CallFunc*)_ast);
      break;
    }

    //
    // Type Constructor
    case AST_TypeConstructor: {
      astdef(TypeConstructor);

      auto& type = ast->typeinfo;

      type = this->check(ast->type);

      debug(for (auto&& elem
                 : ast->elements) { assert(elem.key->kind == AST_Variable); });

      //
      // dont have any members
      if (!type.have_members()) {
        todo_impl;
      }

      //
      // ユーザー定義型
      if (type.kind == TYPE_UserDef) {
        auto usertype = type.userdef_type;

        switch (usertype->kind) {
          case AST_Enum: {
            break;
          }

          case AST_Struct: {
            auto ast_struct = (AST::Struct*)usertype;

            // 初期化子の数が合わない
            //  => エラー
            if (ast->elements.size() != ast_struct->members.size()) {
              Error(ERR_InvalidInitializer, ast, "don't matching member size")
                .emit()
                .exit();
            }

            //
            // 要素を全部チェック
            for (size_t ast_member_index = 0; auto&& elem : ast->elements) {
              //
              // 構造体のメンバへの参照
              auto const& ast_member = ast_struct->members[ast_member_index];

              //
              // 辞書と同じパース処理なので、
              // メンバ名が変数になっていることを確認する
              if (elem.key->kind != AST_Variable) {
                // 変数じゃない場合はエラー
                Error(ERR_InvalidSyntax, elem.key, "expected member name")
                  .emit()
                  .exit();
              }
              else {
                // kind を メンバ変数にする
                elem.key->kind = AST_MemberVariable;

                ((AST::Variable*)elem.key)->index = ast_member_index++;
              }

              // member type
              auto const member_type = this->check(ast_member.type);

              // 名前が合わない
              //  => エラー
              if (ast_member.name != elem.key->token.str) {
                Error(ERR_Undefined, elem.key, "unexpected member name")
                  .emit()
                  .exit();
              }

              this->expect(member_type, elem.value);

              type.members.emplace_back(ast_member.name, member_type);

              // ast_member_index++;
            }

            break;
          }
        }
      }

      _ret = type;
      break;
    }

    case AST_Dict: {
      astdef(Dict);

      _ret = TYPE_Dict;

      if (ast->elements.empty())
        break;

      TypeInfo key_type;
      TypeInfo value_type;

      auto item_iter = ast->elements.begin();

      if (ast->key_type) {
        key_type = this->check(ast->key_type);
        value_type = this->check(ast->value_type);
      }
      else {
        key_type = this->check(item_iter->key);
        value_type = this->check(item_iter->value);
      }

      for (; item_iter != ast->elements.end(); item_iter++) {
        this->expect(key_type, item_iter->key);
        this->expect(value_type, item_iter->value);
      }

      _ret.type_params.emplace_back(std::move(key_type));
      _ret.type_params.emplace_back(std::move(value_type));

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

      auto dest = this->check_as_left(ast->dest);

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
        else {
          type = init_expr_type;
        }
      }
      // 型が指定されてない
      else {
        // => 初期化式がないときエラー
        if (!ast->init) {
          Error(ast, "cannot deduction variable type").emit().exit();
        }

        type = std::move(init_expr_type);
      }

      // 同スコープ内で同じ名前の変数を探す
      auto pvar = scope_emu.lvar.find_var(ast->name);

      // すでに定義済みの同じ名前があって、違う型
      //  => シャドウイングする
      if (pvar && !pvar->type.equals(type)) {
        pvar->type = type;
      }
      // そうでなければ新規追加
      else {
        auto& var = scope_emu.lvar.append(type, ast->name);

        var.index = scope_emu.lvar.variables.size() - 1;
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

      if (!iterable.is_iterable()) {
        Error(ast->iterable, "expected iterable expression").emit().exit();
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
        e.lvar.append(iter, ast->iter->token.str);
      }
      else if (auto x = this->check_as_left(ast->iter); !x.equals(iter)) {
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

      if (auto f = this->find_function(ast->name.str); f && f != ast) {
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
      for (size_t ww = 0; auto&& arg : ast->args) {
        auto& V = S.lvar.append(this->check(arg->type), arg->name);

        V.index = ww++;
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
          Error(ERR_TypeMismatch, *ast->code->list.rbegin(), "type mismatch")
            .emit()
            .exit();
        }
      }
      else if (!res_type.equals(TYPE_None)) {
        if (ast->code->list.empty() || return_types.empty()) {
          Error(ast->token,
                "return type is not none, "
                "but function return nothing")
            .emit();

          Error(ast->result_type, "return type specified with '" +
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
        for (auto it = S.lvar.variables.rbegin(); it != S.lvar.variables.rend();
             it++) {
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

    case AST_IndexRef:
      return this->check_as_left(((AST::IndexRef*)_ast)->expr);
  }

  Error(_ast, "expected lvalue expression").emit().exit();
}

// ------------------------------------------------ //
//  関数呼び出しをチェックする
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
        Error(*arg, "expected '" + formal->to_string() + "' but found '" +
                      actual->to_string() + "'")
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
            Error(*act_arg_it, "too many arguments").emit().exit();
          }

          Error(ast, "too few arguments").emit().exit();
        }
        else if (Q) {  // true!=true で、ループ終了
          break;
        }

        auto arg = *act_arg_it;

        auto aa = this->check((*formal_arg_it)->type);
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
