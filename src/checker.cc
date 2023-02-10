/* ------------------------------------------------------------

Checker

意味解析を行う
型推論、型チェック、など

  ------------------------------------------------------------ */

#include "metro.h"

std::map<AST::Value*, TypeInfo> Checker::value_type_cache;

//
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
      }

      return this->value_type_cache[ast] = ret;
    }

    // 変数
    case AST_Variable: {
      auto ast = (AST::Value*)_ast;

      // インデックス設定すること

      todo_impl;
    }

    //
    // 関数呼び出し
    case AST_CallFunc: {
      auto ast = (AST::CallFunc*)_ast;

      auto const& buitinfunc_list =
        BuiltinFunc::get_builtin_list();

      for( auto&& arg : ast->args ) {
        this->check(arg);
      }

      for( auto&& builtinfunc : buitinfunc_list ) {
        if( ast->name == builtinfunc.name ) {
          ast->is_builtin = true;
          ast->builtin_func = &builtinfunc;
          return builtinfunc.result_type;
        }
      }
      

      Error(ast, "undefined function name")
        .emit()
        .exit();
    }

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

      return left;
    }

    case AST_Let: {

      

      break;
    }

    default:
      todo_impl;
  }

  return TYPE_None;
}


void Checker::check_function_call(AST::CallFunc* ast) {
  // todo
}


//
// 式が有効かどうかを、両辺の型をみて検査する
std::optional<TypeInfo> Checker::is_valid_expr(
  AST::Expr::ExprKind kind, TypeInfo const& lhs, TypeInfo const& rhs) {

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


