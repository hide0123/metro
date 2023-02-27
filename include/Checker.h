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

    size_t offs;

    explicit VariableEmu(std::string_view name, TypeInfo type)
      : name(name),
        type(type),
        offs(0)
    {
    }
  };

  struct ScopeEmu {
    AST::Scope* ast;

    std::vector<VariableEmu> variables;

    VariableEmu* find_var(std::string_view name) {
      for( auto&& var : this->variables )
        if( var.name == name )
          return &var;

      return nullptr;
    }

    explicit ScopeEmu(AST::Scope* ast)
      : ast(ast)
    {
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


  TypeInfo check_as_left(AST::Base* ast);
  

  /**
   * @brief 関数呼び出しが正しいか検査する
   * 
   * @param ast 
   * @return TypeInfo 
   */
  TypeInfo check_function_call(AST::CallFunc* ast);



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



private:

  ScopeEmu& get_cur_scope();

  // 今いる関数を返す
  // 関数の中にいなければ nullptr を返す
  AST::Function* get_cur_func();

  AST::Scope* root;

  std::list<ScopeEmu> scope_list;
  std::list<AST::Function*> function_history;

  size_t variable_stack_offs = 0;

  static std::map<AST::Value*, TypeInfo> value_type_cache;

};
