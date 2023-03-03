#pragma once

#include <optional>
#include <tuple>
#include <list>
#include <map>

#include "AST.h"
#include "TypeInfo.h"

// ---------------------------------------------
//  Sema
// ---------------------------------------------
class Evaluator;
class Sema {
  friend class Evaluator;

  struct FunctionContext {
    AST::Function* func;

    TypeInfo result_type;

    std::map<AST::Return*, TypeInfo> return_stmt_types;
  };

  struct VariableEmu {
    std::string_view name;

    TypeInfo type;

    size_t offs;

    bool is_global = 0;

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

    VariableEmu* find_var(std::string_view name)
    {
      for (auto&& var : this->variables)
        if (var.name == name)
          return &var;

      return nullptr;
    }

    explicit ScopeEmu(AST::Scope* ast)
        : ast(ast)
    {
    }
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
  TypeInfo& sema_index_ref(TypeInfo& type, AST::IndexRef* ast);

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
  std::optional<TypeInfo> is_valid_expr(AST::Expr::ExprKind kind,
                                        TypeInfo const& lhs,
                                        TypeInfo const& rhs);

  /**
   * @brief ユーザー定義関数を探す
   *
   * @param name
   * @return AST::Function*
   */
  AST::Function* find_function(std::string_view name);

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

  ScopeEmu& get_cur_scope();

  // 今いる関数を返す
  // 関数の中にいなければ nullptr を返す
  AST::Function* get_cur_func();

  AST::Scope* root;

  std::list<ScopeEmu> scope_list;
  std::list<AST::Function*> function_history;

  size_t variable_stack_offs = 0;

  // captures
  std::vector<CaptureContext> captures;
  std::vector<ReturnCaptureFunction> return_captures;

  static std::map<AST::Base*, TypeInfo> value_type_cache;
};
