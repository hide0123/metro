#include <cassert>

#include "common.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Checker.h"
#include "Evaluator.h"


static ObjNone objnone;

static ObjNone* none = &objnone;

Evaluator::Evaluator()
{
  ::objnone.ref_count = 1;
}

Evaluator::~Evaluator() {
  
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
  auto& stack = this->call_stack.emplace_back(func);

  return stack;
}

void Evaluator::leave_function(AST::Function* func) {
  auto& stack = *this->call_stack.rbegin();

  debug(assert(stack.ast == func));

  this->call_stack.pop_back();
}

Evaluator::FunctionStack& Evaluator::get_current_func_stack() {
  return *this->call_stack.rbegin();
}

Object* Evaluator::evaluate(AST::Base* _ast) {
  if( !_ast )
    return none;

  switch( _ast->kind ) {
    case AST_None:
    case AST_Function:
      break;

    case AST_Value: {
      return Evaluator::create_object((AST::Value*)_ast);
    }

    case AST_Variable: {
      auto ast = (AST::Variable*)_ast;
      
      return this->object_stack[ast->index];
    }

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

      // 引数をスタックに追加
      for( auto&& obj : args ) {
        this->push_object(obj);
      }

      // 関数実行
      this->evaluate(func->code);

      // 戻り値を取得
      auto result =
        this->get_current_func_stack().result;

      // コールスタック削除
      this->leave_function(func);

      // スタックからオブジェクトを削除
      this->pop_object_with_count(args.size());

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
          elem.op,
          elem.kind,
          ret,
          this->evaluate(elem.ast)
        );
      }

      return ret;
    }

    //
    // 比較式
    case AST_Compare: {
      auto x = (AST::Compare*)_ast;

      auto ret = this->evaluate(x->first);

      for( auto&& elem : x->elements ) {
        if( !Evaluator::compute_compare(
          elem.op,
          elem.kind,
          ret,
          this->evaluate(elem.ast)
        ) ) {
          return new ObjBool(false);
        }
      }

      return new ObjBool(true);
    }

    // scope
    case AST_Scope: {
      auto ast = (AST::Scope*)_ast;

      auto stack_size = this->object_stack.size();

      for( auto&& item : ast->list ) {
        this->evaluate(item);

        if( !this->call_stack.empty() &&
          this->get_current_func_stack().is_returned ) {
          break;
        }
      }

      while( stack_size < this->object_stack.size() )
        this->pop_object();

      gc.clean();

      break;
    }

    case AST_Let: {
      auto ast = (AST::VariableDeclaration*)_ast;

      auto obj = this->evaluate(ast->init);

      obj->ref_count++;

      this->gc.register_object(obj);

      this->object_stack.emplace_back(obj);

      break;
    }

    case AST_Return: {
      auto ast = (AST::Return*)_ast;

      auto& func_stack = this->get_current_func_stack();

      func_stack.result = this->evaluate(ast->expr);

      func_stack.is_returned = true;

      break;
    }

    case AST_If: {
      auto ast = (AST::If*)_ast;

      if( ((ObjBool*)this->evaluate(ast->condition))->value )
        this->evaluate(ast->if_true);
      else if( ast->if_false )
        this->evaluate(ast->if_false);

      break;
    }

    default:
      debug(printf("%d\n",_ast->kind));
      todo_impl;
  }

  return none;
}

Object* Evaluator::compute_expr_operator(
  Token const& op_token,
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
  }

  return ret;
}

bool Evaluator::compute_compare(
  Token const& op_token,
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
