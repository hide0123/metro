/* ------------------------------------------------------------

Checker

意味解析を行う
型推論、型チェック、など

  ------------------------------------------------------------ */

#include "metro.h"

std::map<AST::Value*, TypeInfo> Checker::value_type_cache;

Checker::Checker(AST::Scope* root)
  : root(root),
    call_count(0)
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
      }

      return this->value_type_cache[ast] = ret;
    }

    // 変数
    case AST_Variable: {
      auto ast = (AST::Variable*)_ast;

      // インデックス設定すること

      if( auto res = this->find_variable(ast->token.str); res.has_value() ) {
        auto [emu_iter, stack_index, emu_index]
          = res.value();
        
        ast->index = stack_index;

        return emu_iter->variables[emu_index].type;
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

      return left;
    }

    //
    // 変数定義
    case AST_Let: {
      auto ast = (AST::VariableDeclaration*)_ast;
      
      auto& scope_emu = this->get_cur_scope();

      if( !ast->type ) {
        todo_impl;
      }

      auto type = this->check(ast->type);

      this->check(ast->init);

      if( auto i = scope_emu.find_var(ast->name); i >= 0 ) {
        scope_emu.variables[i].type = type;
      }
      else {
        auto& var = scope_emu.variables.emplace_back();

        var.name = ast->name;
        var.type = type;

        scope_emu.ast->used_stack_size++;
      }

      break;
    }

    //
    // スコープ
    case AST_Scope: {
      this->enter_scope((AST::Scope*)_ast);
      break;
    }

    //
    // 関数
    case AST_Function: {
      auto ast = (AST::Function*)_ast;

      this->call_count++;

      this->check(ast->code);

      this->call_count--;

      return this->check(ast->result_type);
    }

    //
    // return 文
    case AST_Return: {
      auto ast = (AST::Return*)_ast;

      if( this->call_count == 0 ) {
        Error(ast, "cannot use return-statement here")
          .emit()
          .exit();
      }

      return this->check(ast->expr);
    }

    //
    // 型
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

  // なければユーザー定義関数を探す
  if( auto func = this->find_function(ast->name); func ) {
    ast->callee = func;

    // todo: check matching of arguments

    return this->check(func->result_type);
  }

  Error(ast, "undefined function name")
    .emit()
    .exit();
}


void Checker::enter_scope(AST::Scope* ast) {
  
  auto& emu = this->scope_list.emplace_front();

  emu.ast = ast;

  for( auto&& item : ast->list ) {
    this->check(item);
  }

  this->scope_list.pop_front();
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

std::optional<
  std::tuple<std::list<Checker::ScopeEmu>::iterator, size_t, size_t>
>
  Checker::find_variable(std::string_view name) {

  size_t stack_index{ };

  for(
    auto it = this->scope_list.begin();
    it != this->scope_list.end();
    it++, stack_index += it->variables.size()
  ) {
    if( auto i = it->find_var(name); i >= 0 ) {
      return std::make_tuple(it, stack_index, i);
    }

  }

  return std::nullopt;
}


Checker::ScopeEmu& Checker::get_cur_scope() {
  return *this->scope_list.begin();
}
