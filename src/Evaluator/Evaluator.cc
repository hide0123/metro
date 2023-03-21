#include <cassert>
#include <thread>
#include <chrono>

#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Object.h"
#include "BuiltinFunc.h"

#include "Error.h"
#include "Sema.h"
#include "Evaluator.h"

#define astdef(T) auto ast = (AST::T*)_ast

std::map<Object*, bool> Evaluator::allocated_objects;

bool _gc_stopped;

void Evaluator::gc_stop()
{
  _gc_stopped = false;
}

void Evaluator::gc_resume()
{
  _gc_stopped = true;
}

Object::Object(TypeInfo type)
    : type(type),
      ref_count(0),
      no_delete(false)
{
  Evaluator::allocated_objects[this] = 1;
}

Object::~Object()
{
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

  allocated_objects.clear();
}

Object* Evaluator::evaluate(AST::Base* _ast)
{
  if (!_ast)
    return new ObjNone();

  switch (_ast->kind) {
    case AST_None:
    case AST_Function:
    case AST_Struct:
      break;

    case AST_True:
      return new ObjBool(true);

    case AST_False:
      return new ObjBool(false);

    case AST_UnaryMinus: {
      astdef(UnaryOp);

      auto obj = this->evaluate(ast->expr)->clone();

      switch (obj->type.kind) {
        case TYPE_Int:
          ((ObjLong*)obj)->value = -((ObjLong*)obj)->value;
          break;

        case TYPE_Float:
          ((ObjFloat*)obj)->value =
              -((ObjFloat*)obj)->value;
          break;

        default:
          panic("aaa!");
      }

      return obj;
    }

    case AST_UnaryPlus: {
      return this->evaluate(((AST::UnaryOp*)_ast)->expr);
    }

    case AST_Cast: {
      astdef(Cast);

      auto const& cast_to =
          Sema::value_type_cache[ast->cast_to];

      auto obj = this->evaluate(ast->expr);

      switch (cast_to.kind) {
        case TYPE_Int: {
          switch (obj->type.kind) {
            case TYPE_Float:
              return new ObjLong(
                  (float)((ObjFloat*)obj)->value);
          }
          break;
        }
      }

      todo_impl;
    }

    //
    // 即値
    case AST_Value: {
      return Evaluator::create_object((AST::Value*)_ast);
    }

    //
    // Vector
    case AST_Vector: {
      astdef(Vector);

      auto ret = new ObjVector();

      ret->type = Sema::value_type_cache[ast];

      for (auto&& e : ast->elements) {
        ret->append(this->evaluate(e));
      }

      return ret;
    }

    //
    // 左辺値
    case AST_Variable:
    case AST_IndexRef:
      return this->eval_left(_ast)->clone();

    //
    // Dictionary
    case AST_Dict: {
      astdef(Dict);

      auto ret = new ObjDict();
      ret->type = Sema::value_type_cache[_ast];

      for (auto&& elem : ast->elements) {
        ret->append(this->evaluate(elem.key),
                    this->evaluate(elem.value));
      }

      return ret;
    }

    //
    // Range
    case AST_Range: {
      astdef(Range);

      auto begin = this->evaluate(ast->begin);
      auto end = this->evaluate(ast->end);

      return new ObjRange(((ObjLong*)begin)->value,
                          ((ObjLong*)end)->value);
    }

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
      auto& cf = this->enter_function(func);

      // 引数
      auto& vst = this->push_vst();

      for (auto&& obj : args) {
        vst.append_lvar(obj)->ref_count++;
      }

      // 関数実行
      auto ret = this->evaluate(func->code);

      // 戻り値を取得
      auto result = cf.result;

      if (!cf.is_returned) {
        assert(func->code->return_last_expr);

        result = ret;
      }

      assert(result != nullptr);

      for (auto&& obj : args) {
        obj->ref_count--;
      }

      this->pop_vst();

      // コールスタック削除
      this->leave_function();

      this->return_binds[result] = nullptr;

      // 戻り値を返す
      return result;
    }

    case AST_TypeConstructor: {
      astdef(TypeConstructor);

      auto ret = this->default_constructor(ast->typeinfo);

      for (auto&&)

        break;
    }

    //
    // 式
    case AST_Expr: {
      auto x = (AST::Expr*)_ast;

      auto ret = this->evaluate(x->first)->clone();

      ret->no_delete = true;

      for (auto&& elem : x->elements) {
        this->eval_expr_elem(elem, ret);
      }

      ret->no_delete = false;

      return ret;
    }

    //
    // 代入
    case AST_Assign: {
      astdef(Assign);

      auto& dest = this->eval_left(ast->dest);

      dest->ref_count--;

      dest = this->evaluate(ast->expr);
      dest->ref_count++;

      return dest;
    }

    //
    // 比較式
    case AST_Compare: {
      auto x = (AST::Compare*)_ast;

      auto ret = new ObjBool(false);
      auto obj = this->evaluate(x->first);

      for (auto&& elem : x->elements) {
        auto tmp = this->evaluate(elem.ast);

        if (Evaluator::compute_compare(elem.kind, obj, tmp))
          obj = tmp;
        else
          return ret;
      }

      ret->value = true;

      return ret;
    }

    // スコープ
    case AST_Scope: {
      auto ast = (AST::Scope*)_ast;

      if (ast->list.empty())
        break;

      auto& vst = this->push_vst();

      auto iter = ast->list.begin();
      auto const& last = *ast->list.rbegin();

      Object* obj{};

      for (auto&& item : ast->list) {
        if (item == last) {
          obj = this->evaluate(*iter++);
          this->return_binds[obj] = ast;
          break;
        }
        else {
          this->evaluate(*iter++);
        }

        if (vst.is_skipped)
          break;

        if (auto L = this->get_cur_loop();
            L && (L->is_breaked || L->is_continued))
          break;

        if (!this->call_stack.empty() &&
            this->get_current_func_stack().is_returned) {
          break;
        }
      }

      for (auto&& obj : vst.lvar_list) {
        obj->ref_count--;

        this->delete_object(obj);
      }

      this->pop_vst();
      this->clean_obj();

      if (obj)
        return obj;

      break;
    }

    // 変数定義
    case AST_Let: {
      auto ast = (AST::VariableDeclaration*)_ast;

      Object* obj{};

      if (!ast->init) {
        obj = this->default_constructor(
            Sema::value_type_cache[ast->type]);
      }
      else {
        obj = this->evaluate(ast->init);
      }

      obj->ref_count++;

      this->get_vst().append_lvar(obj);

      break;
    }

    case AST_If:
    case AST_Switch:
    case AST_For:
    case AST_Loop:
    case AST_While:
    case AST_DoWhile:
    case AST_Return:
    case AST_Break:
    case AST_Continue:
      if (auto res = this->eval_stmt(_ast); res)
        return res;

      break;

    default:
      alertmsg("evaluation is not implemented yet (kind="
               << (int)_ast->kind << ")");

      todo_impl;
  }

  return new ObjNone;
}

Object*& Evaluator::eval_left(AST::Base* _ast)
{
  switch (_ast->kind) {
    case AST_Variable:
      return this->get_var((AST::Variable*)_ast);

    case AST_IndexRef: {
      astdef(IndexRef);

      return this->eval_index_ref(
          this->eval_left(ast->expr), ast);
    }
  }

  panic("fck, ain't left value. " << _ast << " "
                                  << _ast->kind);
  throw 10;
}

Object*& Evaluator::eval_index_ref(Object*& obj,
                                   AST::IndexRef* ast)
{
  Object** ret = &obj;

  for (auto&& index_ast : ast->indexes) {
    auto obj_index = this->evaluate(index_ast);

    switch ((*ret)->type.kind) {
      case TYPE_Vector: {
        auto& obj_vec = *(ObjVector**)ret;

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
          Error(index_ast, "index out of range")
              .emit()
              .exit();
        }

        ret = &obj_vec->elements[index];
        break;
      }

      case TYPE_Dict: {
        auto& obj_dict = *(ObjDict**)ret;

        for (auto&& item : obj_dict->items) {
          if (item.key->equals(obj_index)) {
            ret = &item.value;
            goto _dict_value_found;
          }
        }

        {
          auto& item = obj_dict->append(
              obj_index,
              this->default_constructor(
                  obj_dict->type.type_params[1]));

          ret = &item.value;
        }

      _dict_value_found:
        break;
      }

      default:
        panic("no index");
    }
  }

  return *ret;
}
