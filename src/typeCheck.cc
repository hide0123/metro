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
      // todo

      auto expr = (AST::Expr*)ast;

      return this->check(expr->first);
    }
  }

  return TYPE_None;
}

//
// 式が有効かどうかを、両辺の型をみて検査する
bool TypeChecker::is_valid_expr(
  ASTKind kind, TypeInfo lhs, TypeInfo rhs) {

  switch( kind ) {
    //
    // add
    case AST_Add: {

      

      break;
    }
  }

  return false;
}


