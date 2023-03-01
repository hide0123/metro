#pragma once

#include <map>
#include "AST.h"
#include "GC.h"

// ---------------------------------------------
//  Evaluator
// ---------------------------------------------
class Evaluator {

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


  //
  // 演算子
  static Object* compute_expr_operator(
    AST::Expr::ExprKind kind,
    Object* left,
    Object* right
  );


  /**
   * @brief 比較する
   * 
   * @return true 
   * @return false 
   */
  static bool compute_compare(
    AST::Compare::CmpKind kind, Object* left, Object* right);


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


  Object* default_constructer(TypeInfo type);

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


  /**
   * @brief オブジェクトをスタックに追加する
   * 
   * @param obj 
   * @return Object*& (追加されたオブジェクトへの参照)
   */
  Object*& push_object(Object* obj);


  /**
   * @brief スタックからオブジェクトを１個削除
   * 
   * @return Object* (スタックから削除されたオブジェクト)
   */
  Object* pop_object();


  /**
   * @brief スタックから指定された数だけオブジェクトを削除する
   */
  void pop_object_with_count(size_t count);


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


  struct var_storage {
    std::map<std::string_view, Object*> vmap;
  };

  std::list<var_storage> vst_list;

  Object*& get_var(std::string_view const& sv) {
    for( auto&& st : this->vst_list )
      if( st.vmap.contains(sv) )
        return st.vmap[sv];

    std::exit(-111);
  }


  //
  // ガベージコレクタ
  GarbageCollector gc;
};
