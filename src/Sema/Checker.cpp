#include "Utils.h"
#include "Debug.h"

#include "AST/AST.h"
#include "Object/Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Sema.h"

#include "ASTWalker.h"

// ----------------------------------
//  check
// ----------------------------------
void Sema::do_check()
{
  TypeRecursionDetector tr{*this};

  for (auto&& x : this->root->list) {
    switch (x->kind) {
      case AST_Function: {
        this->add_function((AST::Function*)x);
        break;
      }

      case AST_Struct:
        tr.walk((AST::Typeable*)x);
        break;

      case AST_Impl: {
        auto ast = (AST::Impl*)x;
        auto type = this->check(ast->type);

        for (auto&& e : ast->list) {
          auto func = (AST::Function*)e;
          auto& ctx = this->add_function(func);

          if (ctx.is_have_self())
            ctx.self_type = type;
        }

        break;
      }
    }
  }

  this->check(this->root);
}

// ------------------------------------------------ //
//  get_subscripted_type
// ------------------------------------------------ //
TypeInfo Sema::check_indexref(AST::IndexRef* ast)
{
  using SubscriptKind = AST::IndexRef::Subscript::Kind;

  TypeInfo type;
  AST::Typeable* usertype = nullptr;

  bool is_first_typename = false;

  if (ast->expr->kind == AST_Variable)
    usertype = this->find_usertype(((AST::Variable*)ast->expr)->name);

  if (!usertype) {
    type = this->check(ast->expr);
    goto check_indexes;
  }
  else {
    is_first_typename = true;
  }

  type.kind = TYPE_UserDef;
  type.userdef_type = usertype;

  if (usertype->kind == AST_Enum) {
    auto ast_enum = (AST::Enum*)usertype;

    is_first_typename = false;

    ast->is_enum = true;
    ast->enum_type = ast_enum;

    auto& sub = ast->indexes[0].ast;

    std::string_view name;
    AST::Base* init = nullptr;
    TypeInfo init_type;

    switch (sub->kind) {
      case AST_Variable: {
        auto x = (AST::Variable*)sub;

        name = x->name;

        break;
      }

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

    //
    // find enumerator
    for (size_t index = 0; auto&& E : ast_enum->enumerators) {
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

        type = TYPE_Enumerator;
        type.userdef_type = ast_enum;

        auto tmp = new AST::NewEnumerator(ast_enum, ast->token);
        tmp->index = index;

        if (init)
          tmp->args.emplace_back(init);

        auto x = ast->expr;

        ast->expr = tmp;
        delete x;

        ast->indexes.erase(ast->indexes.begin());

        goto check_indexes;
      }

      index++;
    }

    Error(ERR_Undefined, sub,
          "enum '" + std::string(ast_enum->name) +
            "' don't have a enumerator '" + std::string(name) + "'")
      .emit()
      .exit();
  }

check_indexes:
  for_indexed(i, index, ast->indexes)
  {
    switch (index.kind) {
      //
      // 配列添字
      case SubscriptKind::SUB_Index: {
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
      case SubscriptKind::SUB_Member: {
        if (!type.have_members()) {
          Error(index.ast,
                "'" + type.to_string() + "' type object don't have any members")
            .emit()
            .exit();
        }

        if (type.kind == TYPE_UserDef) {
          // AST 構造体へのポインタ
          auto pStruct = (AST::Struct*)type.userdef_type;

          // 名前が一致するメンバを探す
          for (auto&& M : pStruct->members) {
            // 一致する名前を発見
            //  --> ループ抜ける (--> @found_member)
            if (M.name == index.ast->token.str) {
              type = this->check(M.type);
              goto found_member;
            }

            ((AST::Variable*)index.ast)->index++;
          }

          // 一致するものがない
          Error(ERR_Undefined, index.ast,
                "struct '" + std::string(pStruct->name) +
                  "' don't have a member '" +
                  std::string(index.ast->token.str) + "'")
            .emit()
            .exit();

        found_member:;
        }
        else {
          // find member of builtin type
          todo_impl;
        }

        break;
      }

      case SubscriptKind::SUB_CallFunc: {
        auto cf = (AST::CallFunc*)index.ast;

        // cf->selftype = type.userdef_type;

        // if (i > 0 && !cf->selftype) {
        //   // todo: built-in type
        //   todo_impl;
        // }

        //
        // if ast->expr is a type name, it's static member-function call.
        cf->is_membercall = i > 0 || !is_first_typename;

        type = this->check_function_call(cf, cf->is_membercall, type);

        this->value_type_cache[cf] = type;

        break;
      }

      default:
        todo_impl;
    }
  }

  if (is_first_typename) {
    auto x = ast->expr;

    ast->expr = ast->indexes[0].ast;
    ast->indexes.erase(ast->indexes.begin());

    delete x;
  }

  while (!ast->indexes.empty() &&
         ast->indexes[0].kind == SubscriptKind::SUB_CallFunc) {
    auto cf = (AST::CallFunc*)ast->indexes[0].ast;

    cf->args.insert(cf->args.begin(), ast->expr);

    ast->expr = cf;
    ast->indexes.erase(ast->indexes.begin());
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

  if (this->value_type_cache.contains(_ast))
    return this->value_type_cache[_ast];

  for (auto&& cap : this->captures) {
    cap.func(_ast);
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

      auto From = this->check(ast->expr);

      if (_ret.equals(From)) {
        Error(ast, "same type, don't need to use cast").emit().exit();
      }

      if (From.equals(TYPE_None)) {
        Error(ast, "cannot cast 'none' to '" + _ret.to_string() + "'")
          .emit()
          .exit();
      }

      if (_ret.equals(TYPE_String))
        goto _cast_done;

      switch (From.kind) {
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

      Error(ast, "cannot cast '" + From.to_string() + "' to '" +
                   _ret.to_string() + "'")
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
      _ret =
        this->check_function_call((AST::CallFunc*)_ast, false, std::nullopt);

      break;
    }

    case AST_NewEnumerator: {
      astdef(NewEnumerator);

      _ret = TYPE_UserDef;
      _ret.userdef_type = ast->ast_enum;

      break;
    }

    //
    // Type Constructor
    case AST_StructConstructor: {
      astdef(StructConstructor);

      auto& pStruct = ast->p_struct;

      auto& type = _ret;
      type = this->check(ast->type);

      if (type.kind != TYPE_UserDef || type.userdef_type->kind != AST_Struct) {
        Error(ast->type, "expected struct name").emit().exit();
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
      for (auto member = pStruct->members.begin();
           auto&& pair : ast->init_pair_list) {
        //
        // メンバ
        auto const& ast_member = *member++;

        // 名前
        if (pair.name != ast_member.name) {
          Error(pair.t_name, "no match member name").emit().exit();
        }

        // 型
        this->expect(this->check(ast_member.type), pair.expr);
      }

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

        if (ast->init) {
          auto tmp = this->check(ast->init);

          if (!tmp.equals(type))
            ast->ignore_initializer = true;

          type = this->expect(type, ast->init);
        }
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
      }
      // そうでなければ新規追加
      else {
        pvar = &scope_emu.lvar.append(type, ast->name);
      }

      pvar->type.is_const = ast->is_const;

      break;
    }

    //
    // return 文
    case AST_Return: {
      auto ast = (AST::Return*)_ast;

      auto func = this->CurFunc;

      if (ast->expr) {
        _ret = this->check(ast->expr);
      }

      if (func) {
        if (!func->result_type) {
          Error(ast->token, "return-statement cannot have an expression")
            .emit();

          if (!_ret.equals(TYPE_None)) {
            Error(func->code->token,
                  "insert '-> " + _ret.to_string() + "' before this token")
              .emit(EL_Note)
              .exit();
          }
        }
        else if (auto t = this->check(func->result_type); !t.equals(_ret)) {
          Error(ast->expr, "expected '" + t.to_string() + "' but found '" +
                             _ret.to_string() + "'")
            .emit()
            .exit();
        }
      }
      else {
        Error(ast, "used 'return' without function").emit().exit();
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
        _ret = this->expect(xx, ast->if_false);

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

      auto fresult = this->find_function(
        ast->name.str, ast->have_self,
        ast->have_self ? std::optional<TypeInfo>(this->check(this->impl_of))
                       : std::nullopt,
        this->make_arg_vector(ast));

      //
      // error: already exists function with same name
      if (fresult.found() && fresult.userdef != ast) {
        Error(ast->name,
              "function '" + std::string(ast->name.str) + "' is already found")
          .emit()
          .exit();
      }

      this->CurFunc = ast;

      // 関数のスコープ　実装があるところ
      auto fn_scope = ast->code;

      fn_scope->of_function = true;

      // スコープ追加
      auto& S = this->enter_scope(fn_scope);

      // self
      if (ast->have_self) {
        S.lvar.append(this->check(this->impl_of), "self");
      }

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

        ast->return_binder = *ast->code->list.rbegin();
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

      this->CurFunc = nullptr;

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

      _ret = TYPE_UserDef;
      _ret.userdef_type = ast;

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

      this->check(ast->type);

      this->cur_impl = ast;
      this->impl_of = ast->type;

      if (auto ut = this->find_usertype(ast->name); ut) {
        this->impl_of = ut;
      }

      for (auto&& x : ast->list) {
        this->check(x);
      }

      this->cur_impl = nullptr;
      this->impl_of = nullptr;

      break;
    }

    //
    // 型
    case AST_Type: {
      auto ast = (AST::Type*)_ast;

      auto& ret = _ret;

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
//  関数呼び出しをチェックする
// ------------------------------------------------ //
TypeInfo Sema::check_function_call(AST::CallFunc* ast, bool have_self,
                                   std::optional<TypeInfo> self)
{
  using FFResult = FunctionFindResult;

#if METRO_DEBUG
  ast->__checked = true;
#endif

  //
  // 引数
  auto args = this->make_arg_vector(ast);

  // 同じ名前の関数を探す
  auto result = this->find_function(ast->name, have_self, self, args);

  //
  // ビルトイン
  if (result.type == FFResult::FN_Builtin) {
    ast->is_builtin = true;
    ast->builtin_func = result.builtin;

    return result.builtin->result_type;
  }

  // ユーザー定義関数
  if (result.type == FFResult::FN_UserDefined) {
    ast->callee = result.userdef;

    return this->check(result.userdef->result_type);
  }

  auto func_name = (self ? self.value().to_string() + "." : "") +
                   std::string(ast->name) + "(" +
                   Utils::String::join(", ", args,
                                       [](auto& aw) {
                                         return aw.typeinfo.to_string();
                                       }) +
                   ")";

  Error(ast, "function '" + func_name + "' not found").emit().exit();
}

TypeInfo Sema::check_struct(AST::Struct* ast)
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

  return TypeInfo::from_usertype(ast);
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
    // mod
    case AST::EX_Mod: {
      if (lhs.equals(TYPE_Int) && rhs.equals(TYPE_Int))
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
