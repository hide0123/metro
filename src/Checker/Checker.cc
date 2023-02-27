/* ------------------------------------------------------------

Checker

意味解析を行う
型推論、型チェック、など

  ------------------------------------------------------------ */

#include "common.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Checker.h"

std::map<AST::Value*, TypeInfo> Checker::value_type_cache;

Checker::Checker(AST::Scope* root)
  : root(root)
{
}

Checker::~Checker() {

}

/**
 * @brief 構文木の意味解析、型チェックなど行う
 * 
 * 
 * @param _ast 
 * @return 評価された _ast の型 (TypeInfo)
 */
TypeInfo Checker::check(AST::Base* _ast) {
  if( !_ast )
    return TYPE_None;

  switch( _ast->kind ) {
    case AST_Value: {
      TypeInfo ret;

      auto ast = (AST::Value*)_ast;

      switch( ast->token.kind ) {
        case TOK_Int:
          ret = TYPE_Int;
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

      return this->value_type_cache[ast] = ret;
    }

    // 変数
    case AST_Variable: {
      auto ast = (AST::Variable*)_ast;

      for(auto&&S:this->scope_list){
        for(auto it=S.variables.rbegin();
        it!=S.variables.rend();it++){
          if(it->name==ast->token.str){
            return it->type;
          }
        }
      }

      Error(ast->token, "undefined variable name")
        .emit()
        .exit();
    }

    //
    // 関数呼び出し
    case AST_CallFunc: {
      return this->check_function_call((AST::CallFunc*)_ast);
    }

    //
    // 式
    case AST_Expr: {
      auto ast = (AST::Expr*)_ast;

      TypeInfo left = this->check(ast->first);

      for( auto&& elem : ast->elements ) {
        auto right = this->check(elem.ast);

        if(
          auto res = this->is_valid_expr(elem.kind, left, right);
          !res ) {
          Error(elem.op, "invalid operator")
            .emit()
            .exit();
        }
        else {
          left = res.value();
        }
      }

      alertmsg("expr;;; "<<left.to_string());

      return left;
    }

    //
    // 比較式
    case AST_Compare: {
      auto ast = (AST::Compare*)_ast;

      TypeInfo left = this->check(ast->first);

      for( auto&& elem : ast->elements ) {
        auto right = this->check(elem.ast);

        if( !left.is_numeric() || !right.is_numeric() ) {
          Error(elem.op, "invalid operator")
            .emit()
            .exit();
        }

        left = right;
      }

      return TYPE_Bool;
    }

    //
    // 変数定義
    case AST_Let: {
      auto ast = (AST::VariableDeclaration*)_ast;
      
      auto& scope_emu = this->get_cur_scope();

      // if( !ast->type ) {
      //   todo_impl;
      // }

      // this->variable_stack_offs++;

      //auto type = this->check(ast->type);
      TypeInfo type;

      auto tt = this->check(ast->init);

      if(ast->type){
        type=this->check(ast->type);
        if(!type.equals(tt))
        Error(ast->init,"mismatched type")
        .emit().exit();
      }
      else
      type=tt;

      if( auto p = scope_emu.find_var(ast->name); p ) {
        // shadowing
        p->type = type;
      }
      else {
        auto& V = scope_emu.variables.emplace_back(
          ast->name,
          type
        );

        V.offs = this->variable_stack_offs++;

        if(auto P=this->get_cur_func();P){
          P->var_count++;
        }

        // scope_emu.ast->used_stack_size++;
      }

      break;
    }

    //
    // return 文
    case AST_Return: {
      auto ast = (AST::Return*)_ast;

      // 関数の中ではない
      if( !this->get_cur_func() ) {
        Error(ast, "cannot use return-statement here")
          .emit()
          .exit();
      }
      // 式がない
      else if( !ast->expr )
        break;

      return this->check(ast->expr);
    }

    //
    // if
    case AST_If: {
      auto ast = (AST::If*)_ast;

      if( !this->check(ast->condition).equals(TYPE_Bool))  {
        Error(ast->condition, "expected boolean expression")
          .emit()
          .exit();
      }

      auto xx = this->check(ast->if_true);

      if(!xx.equals(this->check(ast->if_false)))
        Error(ast,"type mismatch")
          .emit()
          .exit();

      return xx;
    }

    //
    // スコープ
    case AST_Scope: {

      auto ast = (AST::Scope*)_ast;

      auto& emu = this->scope_list.emplace_front(ast);

      auto voffs = this->variable_stack_offs;

      for( auto&& item : ast->list ) {
        auto ww = this->check(item);
      }

      this->variable_stack_offs = voffs;

      this->scope_list.pop_front();
      break;
    }

    //
    // 関数
    case AST_Function: {

      auto ast = (AST::Function*)_ast;

      this->function_history.emplace_front(ast);

      // 関数のスコープ　実装があるところ
      auto fn_scope = ast->code;
      
      // スコープ追加
      auto& S = this->scope_list.emplace_front(fn_scope);

      ast->var_count = ast->args.size();
      

      // 引数追加
      for(auto it=ast->args.rbegin();it!=ast->args.rend();it++)
        S.variables.emplace_back(
          it->name.str,
          this->check(it->type)
        );

      // for( auto&& arg : ast->args ) {
      //   S.variables.emplace_back(
      //     arg.name.str,
      //     this->check(arg.type)
      //   );
      // }

      auto res_type = this->check(ast->result_type);

      for( auto&& x : ast->code->list ) {
        this->check(x);
      }
      
      // スコープ削除
      this->scope_list.pop_front();

      this->function_history.pop_front();

      // return res_type;
      break;
    }

    //
    // 型
    case AST_Type: {
      static struct{TypeKind a;char const* b;}
        const names[]{
        { TYPE_None, "none" },
        { TYPE_Int, "int" },
        { TYPE_Float, "float" },
        { TYPE_Bool, "bool" },
        { TYPE_Char, "char" },
        { TYPE_String, "string" },
        { TYPE_Vector, "vector" },
        { TYPE_Args, "args" },
      };

      auto ast = (AST::Type*)_ast;

      TypeInfo ret;

      for( auto&& x : names )
        if( ast->token.str == x.b ) {
          ret = x.a;
          goto skiperror009;
        }

      Error(ast, "unknown type name")
        .emit()
        .exit();

    skiperror009:;
      ret.is_mutable = ast->is_mutable;

      return ret;
    }

    default:
      debug(printf("%d\n",_ast->kind));
      todo_impl;
  }

  return TYPE_None;
}


TypeInfo Checker::check_function_call(AST::CallFunc* ast) {
  // 組み込み関数リスト取得
  auto const& buitinfunc_list =
    BuiltinFunc::get_builtin_list();

  // 引数
  for( auto&& arg : ast->args ) {
    this->check(arg);
  }

  // 同じ名前のビルトインを探す
  for( auto&& builtinfunc : buitinfunc_list ) {
    if( ast->name == builtinfunc.name ) {
      ast->is_builtin = true;
      ast->builtin_func = &builtinfunc;
      return builtinfunc.result_type;
    }
  }

  // なければユーザー定義関数を探す
  if( auto func = this->find_function(ast->name); func ) {
    ast->callee = func;

    // todo: 引数の一致確認を行う

    // 仮引数 無し
    //if( func->args.empty() ) {
    if( 0 ) {
      // ここのスコープ使わない
      if( !ast->args.empty() ) {
        Error(ast, "too many arguments")
          .emit()
          .exit();
      }
    }
    // 仮引数 有り
    else {
    /// こっちに入る

    // 定義側の引数
    auto formal_arg_it = func->args.begin();

    // 呼び出す方の引数
    auto act_arg_it = ast->args.begin();

    while( 1 ) {
      if( auto Q=formal_arg_it==func->args.end();
          Q!=(act_arg_it==ast->args.end()) ){
      if( Q ) {// 定義側の引数
        Error(*act_arg_it,"too many arguments")
        .emit().exit();
      }
      else{ // 呼び出し
        Error(ast,"too few arguments")
        .emit().exit();
      }
      alertmsg("WTF バグ起きたらこのメッセージ出る てか眠すぎ");
      }
      else if(Q){ // true!=true で、ループ終了
        break;
      }

      auto arg = *act_arg_it;

      auto aa = this->check(formal_arg_it->type);
      auto bb = this->check(*act_arg_it);

      if(!aa.equals(bb)){
        Error(arg,"mismatched type")
        .emit() ;
      }

      formal_arg_it++;
      act_arg_it++;
    }

    } // 引数チェック終了

    return this->check(func->result_type);
  }

  Error(ast, "undefined function name")
    .emit()
    .exit();
}


void Checker::enter_scope(AST::Scope* ast) {

  /*
  alertmsg(ast->token.pos);
  assert(ast != nullptr);

  auto& emu = this->scope_list.emplace_front(ast);

  alertmsg("emu.ast  " << emu.ast);
  alertmsg("ast  " << ast);

  for( auto&& item : ast->list ) {
    this->check(item);
  }

  // assert(emu.ast == ast);

  alertmsg("Checker::enter_scope()");
  alertmsg("used_stack_size: " << emu.ast->used_stack_size);

  this->scope_list.pop_front();
  */
}


/**
 * @brief 演算子に対する両辺の型が適切かどうかチェックする
 * 
 * @param kind 
 * @param lhs 
 * @param rhs 
 * @return std::optional<TypeInfo> 
 */
std::optional<TypeInfo> Checker::is_valid_expr(
  AST::Expr::ExprKind kind,
  TypeInfo const& lhs, TypeInfo const& rhs) {

  if( lhs.equals(TYPE_None) || rhs.equals(TYPE_None) )
    return std::nullopt;

  switch( kind ) {
    //
    // add
    case AST::Expr::EX_Add: {
      if( lhs.equals(rhs) )
        return lhs;

      break;
    }

    //
    // sub
    case AST::Expr::EX_Sub: {
      // remove element from vector
      if( lhs.kind == TYPE_Vector ) {
        if( lhs.type_params[0].equals(rhs) ) {
          return lhs;
        }
      }

      // 数値同士
      if( lhs.is_numeric() && rhs.is_numeric() ) {
        // float を優先する
        return lhs.kind == TYPE_Float ? lhs : rhs;
      }

      break;
    }

    //
    // mul
    case AST::Expr::EX_Mul: {
      if( rhs.is_numeric() )
        return lhs;
      
      break;
    }

    //
    // div
    case AST::Expr::EX_Div: {
      if( lhs.is_numeric() && rhs.is_numeric() )
        return lhs;
    }

    default:
      todo_impl;
  }

  return std::nullopt;
}

bool Checker::check_compare(
  AST::Compare::CmpKind kind, TypeInfo const& lhs, TypeInfo const& rhs) {

  todo_impl;
}

AST::Function* Checker::find_function(std::string_view name) {
  for( auto&& item : this->root->list ) {
    if( item->kind == AST_Function &&
        ((AST::Function*)item)->name.str == name ) {
      return (AST::Function*)item;
    }
  }

  return nullptr;
}

std::optional<
  std::tuple<std::list<Checker::ScopeEmu>::iterator, size_t, size_t>
>
  Checker::find_variable(std::string_view name) {

/*
  size_t stack_index = 0;

  for(
    auto it = this->scope_list.begin();
    it != this->scope_list.end();
    it++, stack_index += it->variables.size()
  ) {
    if( auto p = it->find_var(name); ) {
      return std::make_tuple(
        it, stack_index ? stack_index - 1 : 0, i);
    }
  }
  */

  return std::nullopt;
}


Checker::ScopeEmu& Checker::get_cur_scope() {
  return *this->scope_list.begin();
}

AST::Function* Checker::get_cur_func() {
  return *this->function_history.begin();
}
