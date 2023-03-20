// ------------------------------------------ //
//  semantics analyzer
// ------------------------------------------ //

#pragma once

#include <functional>
#include <optional>
#include <tuple>
#include <list>
#include <map>

#include "AST.h"
#include "TypeInfo.h"

class Evaluator;
class Sema {
  friend class Evaluator;

  struct LocalVar {
    TypeInfo type;
    std::string_view name;

    size_t step;
    size_t index;

    bool is_global = 0;

    explicit LocalVar(TypeInfo const& type,
                      std::string_view name)
        : type(type),
          name(name),
          step(0),
          index(0)
    {
    }
  };

  struct LocalVarList {
    std::vector<LocalVar> variables;

    LocalVar* find_var(std::string_view name)
    {
      for (auto&& var : this->variables)
        if (var.name == name)
          return &var;

      return nullptr;
    }

    LocalVar& append(TypeInfo const& type,
                     std::string_view name)
    {
      return this->variables.emplace_back(type, name);
    }
  };

  struct SemaScope {
    AST::Base* ast;
    LocalVarList lvar;

    bool is_loop;
    bool is_breakable;
    bool is_continueable;

    SemaScope(AST::Base* ast)
        : ast(ast),
          is_loop(false),
          is_breakable(false),
          is_continueable(false)
    {
    }
  };

  struct FunctionContext {
    AST::Function* ast;

    TypeInfo result_type;

    std::map<AST::Return*, TypeInfo> return_stmt_types;
  };

public:
  Sema(AST::Scope* root);
  ~Sema();

  /**
   * @brief 構文木の意味解析、型一致確認など行う
   *
   *
   * @param _ast
   * @return 評価された _ast の型 (TypeInfo)
   */
  TypeInfo check(AST::Base* ast);

  /**
   * @brief 左辺値としてチェック
   *
   * @param ast
   * @return TypeInfo&
   */
  TypeInfo& check_as_left(AST::Base* ast);

  /**
   * @brief インデックス参照
   *
   * @param type
   * @param ast
   * @return TypeInfo&
   */
  TypeInfo& get_subscripted_type(
      TypeInfo& type,
      std::vector<AST::Base*> const& indexes);

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
      AST::ExprKind kind, TypeInfo const& lhs,
      TypeInfo const& rhs);

  /**
   * @brief ユーザー定義関数を探す
   *
   * @param name
   * @return AST::Function*
   */
  AST::Function* find_function(std::string_view name);

  AST::Struct* find_struct(std::string_view name);

  /**
   * @brief ビルトイン関数を探す
   *
   * @param name
   * @return BuiltinFunc const*
   */
  BuiltinFunc const* find_builtin_func(
      std::string_view name);

private:
  using CaptureFunction = std::function<void(AST::Base*)>;
  using ReturnCaptureFunction =
      std::function<void(TypeInfo const&, AST::Base*)>;

  struct CaptureContext {
    CaptureFunction func;

    CaptureContext(CaptureFunction f)
        : func(f)
    {
    }
  };

  void begin_capture(CaptureFunction func);
  void end_capture();

  void begin_return_capture(ReturnCaptureFunction func);
  void end_return_capture();

  SemaScope& get_cur_scope()
  {
    return *this->scope_list.begin();
  }

  SemaScope& enter_scope(AST::Scope* ast)
  {
    return this->scope_list.emplace_front(ast);
  }

  void leave_scope()
  {
    this->scope_list.pop_front();
  }

  // 今いる関数を返す
  // 関数の中にいなければ nullptr を返す
  AST::Function* get_cur_func();

  TypeInfo expect(TypeInfo const& type, AST::Base* ast);

  AST::Scope* root;

  std::list<SemaScope> scope_list;
  std::list<AST::Function*> function_history;

  size_t variable_stack_offs = 0;

  // captures
  std::vector<CaptureContext> captures;
  std::vector<ReturnCaptureFunction> return_captures;

  static std::map<AST::Base*, TypeInfo> value_type_cache;
};
