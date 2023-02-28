#include <cassert>

#include <thread>
#include <chrono>

#include "common.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Checker.h"
#include "Evaluator.h"

#define astdef(T) auto ast=(AST::T*)_ast

static ObjNone objnone;
static ObjNone* none = &objnone;

Evaluator::Evaluator()
{
  ::objnone.ref_count = 1;
}

Evaluator::~Evaluator() {
  
}

Object* Evaluator::evaluate(AST::Base* _ast) {
  debug(
    /*
    std::this_thread::sleep_for(
      std::chrono::milliseconds(100)
    );

    if(!_ast){
      alertmsg("_ast == nullptr")
      return none;
    }
    */
  )

  if( !_ast )
    return none;

  switch( _ast->kind ) {
    case AST_None:
    case AST_Function:
      break;

    // 即値
    case AST_Value: {
      return Evaluator::create_object((AST::Value*)_ast);
    }

    //
    // 左辺値
    case AST_Variable:
    case AST_GlobalVar:
      return this->eval_left(_ast);

    //
    // 関数呼び出し
    case AST_CallFunc: {
      auto ast = (AST::CallFunc*)_ast;

      std::vector<Object*> args;

      // 引数
      for( auto&& arg : ast->args ) {
        args.emplace_back(this->evaluate(arg));
      }

      // 組み込み関数
      if( ast->is_builtin ) {
        return ast->builtin_func->impl(args);
      }

      // ユーザー定義関数
      auto func = ast->callee;

      // コールスタック作成
      this->enter_function(func);

      // 引数
      auto& vst = this->vst_list.emplace_front();
      for( auto xx = func->args.begin(); auto&& obj : args ) {
        vst.vmap[xx->name.str] = obj;
      }

      // 関数実行
      this->evaluate(func->code);

      // 戻り値を取得
      auto result =
        this->get_current_func_stack().result;

      if( !result ) {
        result=none;
      }

      assert(result != nullptr);

      this->vst_list.pop_front();

      // コールスタック削除
      this->leave_function();

      // 戻り値を返す
      return result;
    }

    //
    // 式
    case AST_Expr: {
      auto x = (AST::Expr*)_ast;

      auto ret = this->evaluate(x->first);

      for( auto&& elem : x->elements ) {
        ret = Evaluator::compute_expr_operator(
          elem.kind,
          ret,
          this->evaluate(elem.ast)
        );
      }

      return ret;
    }

    //
    // 代入
    case AST_Assign: {
      astdef(Assign);

      return
        this->eval_left(ast->dest)
          = this->evaluate(ast->expr);
    }

    //
    // 比較式
    case AST_Compare: {
      auto x = (AST::Compare*)_ast;

      auto ret = this->evaluate(x->first);
      Object* xxx{};

      for( auto&& elem : x->elements ) {
        if( !Evaluator::compute_compare(
          elem.kind,
          ret,
          xxx = this->evaluate(elem.ast)
        ) ) {
          return new ObjBool(false);
        }

        ret = xxx;
      }

      return new ObjBool(true);
    }

    // スコープ
    case AST_Scope: {
      auto ast = (AST::Scope*)_ast;

      this->vst_list.emplace_front();

      for( auto&& item : ast->list ) {
        this->evaluate(item);

        if( !this->call_stack.empty() &&
          this->get_current_func_stack().is_returned ) {
          
          break;
        }
      }

      this->vst_list.pop_front();

      gc.clean();

      break;
    }

    // 変数定義
    case AST_Let: {
      auto ast = (AST::VariableDeclaration*)_ast;

      auto obj = this->evaluate(ast->init);

      obj->ref_count++;

      this->gc.register_object(obj);

      auto& vst = *this->vst_list.begin();

      vst.vmap[ast->name] = obj;

      break;
    }

    //
    // Return
    case AST_Return: {
      auto ast = (AST::Return*)_ast;

      auto& fs = this->get_current_func_stack();

      if( ast->expr )
        fs.result = this->evaluate(ast->expr);
      else
        fs.result = none;

      // フラグ有効化
      fs.is_returned = true;

      assert(fs.result != nullptr);
      break;
    }

    //
    // If
    case AST_If: {
      auto ast = (AST::If*)_ast;

      if( ((ObjBool*)this->evaluate(ast->condition))->value )
        this->evaluate(ast->if_true);
      else if( ast->if_false )
        this->evaluate(ast->if_false);

      break;
    }

    default:
      alertmsg(
        "evaluation is not implemented yet (kind="
        << _ast->kind << ")"
      );

      todo_impl;
  }

  return none;
}

Object*& Evaluator::eval_left(AST::Base* _ast) {

  switch(_ast->kind){
    case AST_Variable: {
      astdef(Variable);

      return this->get_var(ast->token.str);
    }
  }

  panic("fck, ain't left value. %p", _ast);
  throw 10;
}


Object* Evaluator::compute_expr_operator(
  AST::Expr::ExprKind kind,
  Object* left,
  Object* right
) {
  using EX = AST::Expr::ExprKind;

  auto ret = left->clone();

  switch( kind ) {
    case EX::EX_Add: {
      switch( left->type.kind ) {
        case TYPE_Int:
          ((ObjLong*)ret)->value += ((ObjLong*)right)->value;
          break;
      }
      break;
    }

    case EX::EX_Sub: {
      switch( left->type.kind ) {
        case TYPE_Int:
          ((ObjLong*)ret)->value -= ((ObjLong*)right)->value;
          break;
      }
      break;
    }

    case EX::EX_Mul: {
      switch( left->type.kind ) {
        case TYPE_Int:
          ((ObjLong*)ret)->value *= ((ObjLong*)right)->value;
          break;
      }
      break;
    }

    default:
      todo_impl;
  }

  return ret;
}

bool Evaluator::compute_compare(
  AST::Compare::CmpKind kind, Object* left, Object* right) {

  using CK = AST::Compare::CmpKind;

  float a = left->type.kind == TYPE_Int
    ? ((ObjLong*)left)->value : ((ObjFloat*)left)->value;
    
  float b = right->type.kind == TYPE_Int
    ? ((ObjLong*)right)->value : ((ObjFloat*)right)->value;
    
  switch( kind ) {
    case CK::CMP_LeftBigger:
      return a > b;
    
    case CK::CMP_RightBigger:
      return a < b;

    case CK::CMP_LeftBigOrEqual:
      return a >= b;

    case CK::CMP_RightBigOrEqual:
      return a <= b;

    case CK::CMP_Equal:
      return a == b;
    
    case CK::CMP_NotEqual:
      return a != b;
  }

  return false;
}
  
Object*& Evaluator::push_object(Object* obj) {
  return this->object_stack.emplace_back(obj);
}

Object* Evaluator::pop_object() {
  auto obj = *this->object_stack.rbegin();

  this->object_stack.pop_back();

  return obj;
}

void Evaluator::pop_object_with_count(size_t count) {
  for( size_t i = 0; i < count; i++ ) {
    this->pop_object();
  }
}

/**
 * @brief 即値・リテラルの AST からオブジェクトを作成する
 * 
 * @note すでに作成済みのものであれば、既存のものを返す
 * 
 * @param ast 
 * @return 作成されたオブジェクト (Object*)
 */
Object* Evaluator::create_object(AST::Value* ast) {
  auto type = Checker::value_type_cache[ast];

  auto& obj = this->immediate_objects[ast];

  if( obj )
    return obj;

  switch( type.kind ) {
    case TYPE_Int:
      obj = new ObjLong(std::stoi(ast->token.str.data()));
      break;

    case TYPE_String:
      obj = new ObjString(Utils::String::to_wstr(
        std::string(ast->token.str)
      ));

      break;

    default:
      debug(
        std::cout << type.to_string() << std::endl
      );

      todo_impl;
  }

  assert(obj != nullptr);

  return obj;
}

Evaluator::FunctionStack& Evaluator::enter_function(AST::Function* func) {
  return this->call_stack.emplace_front(func);
}

void Evaluator::leave_function() {
  this->call_stack.pop_front();
}

Evaluator::FunctionStack& Evaluator::get_current_func_stack() {
  return *this->call_stack.begin();
}
