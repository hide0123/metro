/* ------------------------------------------------------------

Checker

意味解析を行う
型推論、型チェック、など

  ------------------------------------------------------------ */

#include "metro.h"

std::map<AST::Value*, TypeInfo> Checker::value_type_cache;

Checker::Checker(AST::Scope* root)
  : root(root)
{
}

Checker::~Checker() {

}

/**
 * @brief _ast の型を評価する
 * 
 * 
 * @param _ast 
 * @return TypeInfo 
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

    //
    // スコープ
    case AST_Scope: {
      auto ast = (AST::Scope*)_ast;

      for( auto&& item : ast->list ) {
        this->check(item);
      }

      break;
    }

    //
    // 関数
    case AST_Function: {

      break;
    }

    case AST_Type: {
      auto ast = (AST::Type*)_ast;

      if( ast->token.str == "int" ) return TYPE_Int;

      Error(ast, "unknown type name")
        .emit()
        .exit();
    }

    default:
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

  if( auto func = this->find_function(ast->name); func ) {
    ast->callee = func;

    // todo: check matching of arguments

    return this->check(func->result_type);
  }

  Error(ast, "undefined function name")
    .emit()
    .exit();
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

AST::Function* Checker::find_function(std::string_view name) {
  for( auto&& item : this->root->list ) {
    if( item->kind == AST_Function &&
        ((AST::Function*)item)->name.str == name ) {
      return (AST::Function*)item;
    }
  }

  return nullptr;
}