#pragma once

#include <optional>
#include <tuple>
#include <list>
#include <map>

#include "AST.h"
#include "TypeInfo.h"

// ---------------------------------------------
//  Checker
// ---------------------------------------------
class Evaluator;
class Checker {
  friend class Evaluator;

  struct FunctionContext {
    AST::Function* func;

    TypeInfo result_type;
  };

  struct VariableEmu {
    std::string_view name;

    TypeInfo type;
  };

  struct ScopeEmu {
    AST::Scope* ast;

    std::vector<VariableEmu> variables;

    int find_var(std::string_view name) {
      for( int i = 0; auto&& var : this->variables ) {
        if( var.name == name )
          return i;

        i++;
      }

      return -1;
    }
  };

public:

  Checker(AST::Scope* root);
  ~Checker();


  /**
   * @brief 構文木の意味解析、型一致確認など行う
   * 
   * 
   * @param _ast 
   * @return 評価された _ast の型 (TypeInfo)
   */
  TypeInfo check(AST::Base* ast);
  

  /**
   * @brief 関数呼び出しが正しいか検査する
   * 
   * @param ast 
   * @return TypeInfo 
   */
  TypeInfo check_function_call(AST::CallFunc* ast);


  /**
   * @brief スコープ
   * 
   * @param ast 
   */
  void enter_scope(AST::Scope* ast);
  

  /**
   * @brief  式の両辺の型が正しいかどうか検査する
   * 
   * @param kind 
   * @param lhs 
   * @param rhs 
   * @return std::optional<TypeInfo> 
   */
  std::optional<TypeInfo> is_valid_expr(
    AST::Expr::ExprKind kind, TypeInfo const& lhs, TypeInfo const& rhs);


  /**
   * @brief ユーザー定義関数を探す
   * 
   * @param name 
   * @return AST::Function* 
   */
  AST::Function* find_function(std::string_view name);


  /**
   * @brief 変数を探す
   * 
   * @param name 
   * @return std::optional<
   * std::tuple<std::list<ScopeEmu>::iterator, size_t, size_t>
   * > 
   */
  std::optional<
    std::tuple<
      std::list<ScopeEmu>::iterator,  // iter to ScopeEmu
      size_t, // in Evaluator, index to object n stack
      size_t  // index in ScopeEmu
    >
  >
    find_variable(std::string_view name);


private:

  ScopeEmu& get_cur_scope();

  AST::Scope* root;

  int call_count;

  std::list<ScopeEmu> scope_list;

  static std::map<AST::Value*, TypeInfo> value_type_cache;

};
