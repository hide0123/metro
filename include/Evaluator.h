#pragma once

#include <map>
#include "AST.h"
#include "GC.h"

// ---------------------------------------------
//  Evaluator
// ---------------------------------------------
class Evaluator {
  friend struct Object;

  struct FunctionStack {
    AST::Function const* ast;

    Object* result;

    bool is_returned;

    explicit FunctionStack(AST::Function const* ast)
        : ast(ast),
          result(nullptr),
          is_returned(false)
    {
    }
  };

  struct var_storage {
    std::vector<Object*> lvar_list;

    bool is_skipped = 0;

    Object*& get_lvar(size_t index)
    {
      return this->lvar_list[index];
    }

    Object*& append_lvar(Object* obj = nullptr)
    {
      return this->lvar_list.emplace_back(obj);
    }
  };

  struct LoopStack {
    var_storage& vs;
    bool is_breaked;

    LoopStack(var_storage& vs)
        : vs(vs),
          is_breaked(false)
    {
    }
  };

public:
  Evaluator();
  ~Evaluator();

  /**
   * @brief 構文木を評価する（実行）
   *
   * @param ast
   * @return Object*
   */
  Object* evaluate(AST::Base* ast);

  /**
   * @brief AST を左辺値の式として評価する
   *
   * @param ast
   * @return Object*&
   */
  Object*& eval_left(AST::Base* ast);

  /**
   * @brief
   *
   * @param obj
   * @param ast
   * @return Object*&
   */
  Object*& eval_index_ref(Object*& obj, AST::IndexRef* ast);

  /**
   * @brief 演算子
   *
   * @param kind
   * @param left
   * @param right
   * @return Object*
   */
  static Object* compute_expr_operator(AST::ExprKind kind,
                                       Token const& token,
                                       Object* left,
                                       Object* right);

  /**
   * @brief 比較する
   *
   * @return true
   * @return false
   */
  static bool compute_compare(AST::CmpKind kind, Object* left,
                              Object* right);

private:
  /**
   * @brief 即値・リテラルの構文木からオブジェクトを作成する
   *
   * @note すでに作成済みであればそれを返す
   *
   * @param ast
   * @return Object*
   */
  Object* create_object(AST::Value* ast);

  /**
   * @brief 型情報から初期値を作成する
   *
   * @param type
   * @return Object*
   */
  Object* default_constructer(TypeInfo const& type);

  /**
   * @brief
   *
   * @param func
   * @return FunctionStack&
   */
  FunctionStack& enter_function(AST::Function* func);

  /**
   * @brief
   *
   * @param func
   */
  void leave_function();

  /**
   * @brief 現在のコールスタックを取得する
   *
   * @return FunctionStack&
   */
  FunctionStack& get_current_func_stack();

  void delete_object(Object* p);

  void clean_obj();

  var_storage& push_vst()
  {
    return this->vst_list.emplace_back();
  }

  void pop_vst()
  {
    this->vst_list.pop_back();
  }

  var_storage& get_vst()
  {
    return *this->vst_list.rbegin();
  }

  LoopStack* get_cur_loop()
  {
    if (this->loop_stack.empty())
      return nullptr;

    return &*this->loop_stack.begin();
  }

  Object*& get_var(AST::Variable* ast)
  {
    auto it = this->vst_list.rbegin();

    for (size_t i = 0; i < ast->step; i++)
      it++;

    return it->get_lvar(ast->index);
  }

  //
  // オブジェクトスタック
  // 変数・引数で使う
  std::vector<Object*> object_stack;

  //
  // コールスタック
  // 関数呼び出し用
  std::list<FunctionStack> call_stack;

  //
  // 即値・リテラル
  std::map<AST::Value*, Object*> immediate_objects;

  std::list<var_storage> vst_list;
  std::list<LoopStack> loop_stack;
  std::map<Object*, AST::Return*> return_binds;

  static std::map<Object*, bool> allocated_objects;
};
