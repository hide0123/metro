#include "lcc.h"

//
// 型チェック
TypeInfo TypeChecker::check(AST::Base* ast) {
  if( !ast )
    return TYPE_None;

  switch( ast->kind ) {
    case AST_Value: {
      switch( ((AST::Value*)ast)->token.kind ) {
        case TOK_Int: return TYPE_Int;
        case TOK_Float: return TYPE_Float;
      }

      todo_impl;
    }
    
    case AST_Variable:
      todo_impl;

    case AST_Expr: {
      auto expr = (AST::Expr*)ast;

      TypeInfo left = this->check(expr->first);

      for( auto&& elem : expr->elements ) {
        auto const& right = this->check(elem.ast);

        if( auto res = this->is_valid_expr(elem.kind, left, right);
            res == std::nullopt ) {
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
  }

  return TYPE_None;
}

//
// 式が有効かどうかを、両辺の型をみて検査する
std::optional<TypeInfo> TypeChecker::is_valid_expr(
  ASTKind kind, TypeInfo const& lhs, TypeInfo const& rhs) {

  if( lhs.equals(TYPE_None) || rhs.equals(TYPE_None) )
    return std::nullopt;

  switch( kind ) {
    //
    // add
    case AST_Add: {
      if( lhs.equals(rhs) )
        return lhs;
      
      break;
    }

    //
    // sub
    case AST_Sub: {
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
    case AST_Mul: {
      if( rhs.is_numeric() )
        return lhs;
      
      break;
    }

    //
    // div
    case AST_Div: {
      if( lhs.is_numeric() && rhs.is_numeric() )
        return lhs;
    }

    default:
      todo_impl;
  }

  return std::nullopt;
}


