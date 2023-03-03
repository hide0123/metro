#include <cassert>

#include <thread>
#include <chrono>

#include "common.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Sema.h"
#include "Evaluator.h"

#include "debug/alert.h"

#define astdef(T) auto ast = (AST::T*)_ast

Object* Evaluator::none;
std::map<Object*, bool> Evaluator::allocated_objects;

static bool _gc_stopped;

#if METRO_DEBUG
std::map<Object*, int> _all_obj;
#endif

Object::Object(TypeInfo type)
    : type(type),
      ref_count(0),
      no_delete(false)
{
  alert_ctor;

  Evaluator::allocated_objects[this] = 1;

  debug(_all_obj[this] = 1);
}

Object::~Object()
{
  alert_dtor;

  debug(_all_obj[this] = 0);
}

void Evaluator::delete_object(Object* p)
{
  if (_gc_stopped)
    return;

  if (this->return_binds[p] != nullptr)
    return;

  if (allocated_objects[p] == 0)
    return;

  if (p->ref_count == 0 && !p->no_delete) {
    alertmsg("delete_object(): " << p);
    allocated_objects[p] = 0;
    delete p;
  }
}

void Evaluator::clean_obj()
{
  for (auto&& [p, b] : allocated_objects) {
    delete_object(p);
  }
}

Evaluator::Evaluator()
{
}

Evaluator::~Evaluator()
{
  for (auto&& [x, y] : this->immediate_objects) {
    delete y;
  }
}

Object* Evaluator::evaluate(AST::Base* _ast)
{
  if (!_ast)
    return new ObjNone();

  switch (_ast->kind) {
    case AST_None:
    case AST_Function:
      break;

    // 即値
    case AST_Value: {
      return Evaluator::create_object((AST::Value*)_ast);
    }

    case AST_Array: {
      astdef(Array);

      auto ret = new ObjVector();

      ret->type = Sema::value_type_cache[ast];

      for (auto&& e : ast->elements) {
        ret->elements.emplace_back(this->evaluate(e));
      }

      return ret;
    }

    //
    // 左辺値
    case AST_Variable:
    case AST_GlobalVar:
    case AST_IndexRef:
      return this->eval_left(_ast);

    case AST_Dict: {
      astdef(Dict);

      auto ret = new ObjDict();
      ret->type = Sema::value_type_cache[_ast];

      for (auto&& elem : ast->elements) {
        ret->items.emplace_back(this->evaluate(elem.key),
                                this->evaluate(elem.value));
      }

      return ret;
    }

    case AST_Range: {
      astdef(Range);

      auto begin = this->evaluate(ast->begin);
      auto end = this->evaluate(ast->end);

      return new ObjRange(((ObjLong*)begin)->value,
                          ((ObjLong*)end)->value);
    }

    // case AST_IndexRef: {
    //   astdef(IndexRef);

    //   return this->eval_index_ref(this->evaluate(ast->expr),
    //                               ast);
    // }

    //
    // 関数呼び出し
    case AST_CallFunc: {
      auto ast = (AST::CallFunc*)_ast;

      std::vector<Object*> args;

      // 引数
      for (auto&& arg : ast->args) {
        args.emplace_back(this->evaluate(arg));
      }

      // 組み込み関数
      if (ast->is_builtin) {
        return ast->builtin_func->impl(args);
      }

      // ユーザー定義関数
      auto func = ast->callee;

      // コールスタック作成
      this->enter_function(func);

      // 引数
      auto& vst = this->vst_list.emplace_front();
      for (auto xx = func->args.begin(); auto&& obj : args) {
        vst.vmap[xx->name.str] = obj;

        obj->ref_count++;
      }

      // 関数実行
      this->evaluate(func->code);

      // 戻り値を取得
      auto result = this->get_current_func_stack().result;

      if (!result) {
        result = new ObjNone();
      }

      assert(result != nullptr);

      for (auto&& obj : args) {
        obj->ref_count--;
      }

      this->vst_list.pop_front();

      // コールスタック削除
      this->leave_function();

      this->return_binds[result] = nullptr;

      // debug(

      //     alert;
      //     alertmsg(COL_YELLOW "ret: " << result->to_string());

      //     Error(ast, "returnnn " + result->to_string())
      //         .emit(Error::EL_Note);

      // );

      // 戻り値を返す
      return result;
    }

    //
    // 式
    case AST_Expr: {
      auto x = (AST::Expr*)_ast;

      auto ret = this->evaluate(x->first)->clone();

      for (auto&& elem : x->elements) {
        ret = Evaluator::compute_expr_operator(
            elem.kind, ret, this->evaluate(elem.ast));
      }

      return ret;
    }

    //
    // 代入
    case AST_Assign: {
      astdef(Assign);

      return this->eval_left(ast->dest) =
                 this->evaluate(ast->expr);
    }

    //
    // 比較式
    case AST_Compare: {
      auto x = (AST::Compare*)_ast;

      auto ret = this->evaluate(x->first);
      Object* xxx{};

      for (auto&& elem : x->elements) {
        if (!Evaluator::compute_compare(
                elem.kind, ret,
                xxx = this->evaluate(elem.ast))) {
          return new ObjBool(false);
        }

        ret = xxx;
      }

      return new ObjBool(true);
    }

    // スコープ
    case AST_Scope: {
      auto ast = (AST::Scope*)_ast;

      auto& vst = this->vst_list.emplace_front();

      for (auto&& item : ast->list) {
        this->evaluate(item);

        if (vst.is_skipped)
          break;

        if (auto L = this->get_cur_loop(); L && L->is_breaked)
          break;

        if (!this->call_stack.empty() &&
            this->get_current_func_stack().is_returned) {
          break;
        }
      }

      for (auto&& [name, obj] : vst.vmap) {
        alertmsg("lvar " << name << ": " << obj
                         << " ref_count=" << obj->ref_count);

        obj->ref_count--;

        if (obj->ref_count == 0) {
          this->delete_object(obj);
        }
      }

      this->vst_list.pop_front();
      this->clean_obj();

      break;
    }

    // 変数定義
    case AST_Let: {
      auto ast = (AST::VariableDeclaration*)_ast;

      auto& vst = *this->vst_list.begin();

      auto obj = this->evaluate(ast->init);

      obj->ref_count++;

      vst.vmap[ast->name] = obj;

      break;
    }

    //
    // Return
    case AST_Return: {
      auto ast = (AST::Return*)_ast;

      auto& fs = this->get_current_func_stack();

      if (ast->expr) {
        auto _flag_b = _gc_stopped;

        _gc_stopped = true;

        fs.result = this->evaluate(ast->expr);

        _gc_stopped = _flag_b;

        this->return_binds[fs.result] = ast;
      }
      else
        fs.result = new ObjNone();

      // フラグ有効化
      fs.is_returned = true;

      // vst.is_skipped = 1;

      assert(fs.result != nullptr);
      break;
    }

    //
    // break / continue
    case AST_Break:
      this->get_cur_loop()->is_breaked = true;

    case AST_Continue:
      this->get_cur_loop()->vs.is_skipped = true;
      break;

    //
    // If
    case AST_If: {
      auto ast = (AST::If*)_ast;

      if (((ObjBool*)this->evaluate(ast->condition))->value)
        this->evaluate(ast->if_true);
      else if (ast->if_false)
        this->evaluate(ast->if_false);

      break;
    }

    //
    // for-loop
    case AST_For: {
      astdef(For);

      auto _obj = this->evaluate(ast->iterable);
      _obj->ref_count++;

      auto& v = this->vst_list.emplace_front();

      auto& loop = this->loop_stack.emplace_front(v);

      Object** p_iter = nullptr;

      if (ast->iter->kind == AST_Variable) {
        p_iter = &v.vmap[ast->iter->token.str];
      }

      switch (_obj->type.kind) {
        case TYPE_Range: {
          auto& iter = *(ObjLong**)p_iter;
          auto obj = (ObjRange*)_obj;

          iter = new ObjLong(obj->begin);
          iter->no_delete = 1;

          while (iter->value < obj->end) {
            alert;

            alertmsg(iter->value);

            v.is_skipped = 0;

            this->evaluate(ast->code);

            if (loop.is_breaked) {
              break;
            }

            iter->value++;
          }

          delete iter;

          break;
        }

        default:
          todo_impl;
      }

      // if (ast->iter->kind == AST_Variable) {
      //   (*p_iter)->
      // }

      this->loop_stack.pop_front();
      this->vst_list.pop_front();

      _obj->ref_count--;

      break;
    }

    default:
      alertmsg("evaluation is not implemented yet (kind="
               << _ast->kind << ")");

      todo_impl;
  }

  return new ObjNone;
}

Object*& Evaluator::eval_left(AST::Base* _ast)
{
  switch (_ast->kind) {
    case AST_Variable: {
      astdef(Variable);

      return this->get_var(ast->token.str);
    }

    case AST_IndexRef: {
      astdef(IndexRef);

      return this->eval_index_ref(this->eval_left(ast->expr),
                                  ast);
    }
  }

  panic("fck, ain't left value. " << _ast);
  throw 10;
}

Object*& Evaluator::eval_index_ref(Object*& obj,
                                   AST::IndexRef* ast)
{
  Object** ret{};

  for (auto&& index_ast : ast->indexes) {
    auto obj_index = this->evaluate(index_ast);

    switch (obj->type.kind) {
      case TYPE_Vector: {
        auto obj_vec = (ObjVector*)obj;

        size_t index = 0;

        switch (obj_index->type.kind) {
          case TYPE_Int:
            index = ((ObjLong*)obj_index)->value;
            break;

          case TYPE_USize:
            index = ((ObjUSize*)obj_index)->value;
            break;

          default:
            panic("int or usize??aa");
        }

        if (index >= obj_vec->elements.size()) {
          Error(index_ast, "index out of range").emit().exit();
        }

        ret = &obj_vec->elements[index];
        break;
      }

      case TYPE_Dict: {
        auto obj_dict = (ObjDict*)obj;

        for (auto&& item : obj_dict->items) {
          if (item.key->equals(obj_index)) {
            obj = item.value;
            goto _dict_value_found;
          }
        }

        ret = &obj_dict->items
                   .emplace_back(
                       obj_index,
                       this->default_constructer(
                           obj_dict->type.type_params[1]))
                   .value;

      _dict_value_found:
        break;
      }

      default:
        panic("no index");
    }
  }

  return *ret;
}

Object* Evaluator::default_constructer(TypeInfo const& type)
{
  switch (type.kind) {
    case TYPE_Int:
      return new ObjLong;

    case TYPE_USize:
      return new ObjUSize;

    case TYPE_Float:
      return new ObjFloat;

    case TYPE_String:
      return new ObjString;

    case TYPE_Dict: {
      auto ret = new ObjDict;

      ret->type = type;

      return ret;
    }
  }

  panic("u9r043290");
}

Object* Evaluator::compute_expr_operator(
    AST::Expr::ExprKind kind, Object* left, Object* right)
{
  using EX = AST::Expr::ExprKind;

  // auto ret = left->clone();
  auto ret = left;

  switch (kind) {
    case EX::EX_Add: {
      switch (left->type.kind) {
        case TYPE_Int:
          ((ObjLong*)ret)->value += ((ObjLong*)right)->value;
          break;

        default:
          todo_impl;
      }
      break;
    }

    case EX::EX_Sub: {
      switch (left->type.kind) {
        case TYPE_Int:
          ((ObjLong*)ret)->value -= ((ObjLong*)right)->value;
          break;

        default:
          todo_impl;
      }
      break;
    }

    case EX::EX_Mul: {
      switch (left->type.kind) {
        case TYPE_Int:
          ((ObjLong*)ret)->value *= ((ObjLong*)right)->value;
          break;

        default:
          todo_impl;
      }
      break;
    }

    default:
      todo_impl;
  }

  return ret;
}

bool Evaluator::compute_compare(AST::Compare::CmpKind kind,
                                Object* left, Object* right)
{
  using CK = AST::Compare::CmpKind;

  float a = left->type.kind == TYPE_Int
                ? ((ObjLong*)left)->value
                : ((ObjFloat*)left)->value;

  float b = right->type.kind == TYPE_Int
                ? ((ObjLong*)right)->value
                : ((ObjFloat*)right)->value;

  switch (kind) {
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

Object*& Evaluator::push_object(Object* obj)
{
  return this->object_stack.emplace_back(obj);
}

Object* Evaluator::pop_object()
{
  auto obj = *this->object_stack.rbegin();

  this->object_stack.pop_back();

  return obj;
}

void Evaluator::pop_object_with_count(size_t count)
{
  for (size_t i = 0; i < count; i++) {
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
Object* Evaluator::create_object(AST::Value* ast)
{
  auto type = Sema::value_type_cache[ast];

  auto& obj = this->immediate_objects[ast];

  if (obj)
    return obj;

  switch (type.kind) {
    case TYPE_Int:
      obj = new ObjLong(std::stoi(ast->token.str.data()));
      break;

    case TYPE_Float:
      obj = new ObjFloat(std::stof(ast->token.str.data()));
      break;

    case TYPE_USize:
      obj = new ObjUSize(std::stoi(ast->token.str.data()));
      break;

    case TYPE_String:
      obj = new ObjString(
          Utils::String::to_wstr(std::string(ast->token.str)));

      break;

    default:
      debug(std::cout << type.to_string() << std::endl);

      todo_impl;
  }

  assert(obj != nullptr);

  obj->no_delete = 1;

  return obj;
}

Evaluator::FunctionStack& Evaluator::enter_function(
    AST::Function* func)
{
  return this->call_stack.emplace_front(func);
}

void Evaluator::leave_function()
{
  this->call_stack.pop_front();
}

Evaluator::FunctionStack& Evaluator::get_current_func_stack()
{
  return *this->call_stack.begin();
}
