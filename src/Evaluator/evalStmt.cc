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

extern bool _gc_stopped;

Object* Evaluator::eval_stmt(AST::Base* _ast)
{
  switch (_ast->kind) {
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

      assert(fs.result != nullptr);
      break;
    }

    //
    // break / continue
    case AST_Break:
      this->get_cur_loop()->is_breaked = true;
      break;

    case AST_Continue:
      this->get_cur_loop()->is_continued = true;
      break;

    //
    // If
    case AST_If: {
      auto ast = (AST::If*)_ast;

      if (((ObjBool*)this->evaluate(ast->condition))->value)
        return this->evaluate(ast->if_true);
      else if (ast->if_false)
        return this->evaluate(ast->if_false);

      break;
    }

    //
    // switch
    case AST_Switch: {
      astdef(Switch);

      auto item = this->evaluate(ast->expr);

      for (auto&& c : ast->cases) {
        alert;

        auto cond = this->evaluate(c->cond);

        if (cond->type.equals(TYPE_Bool)) {
          if (!((ObjBool*)cond)->value)
            continue;
        }

        if (!cond->equals(item))
          continue;

        this->evaluate(c->scope);
        break;
      }

      break;
    }

    //
    // loop
    case AST_Loop: {
      auto& vst = this->get_vst();
      auto& loop = this->loop_stack.emplace_front(vst);

      while (true) {
        this->evaluate(((AST::Loop*)_ast)->code);

        if (loop.is_breaked)
          break;

        loop.is_continued = false;
      }

      this->loop_stack.pop_front();
      break;
    }

    //
    // for-loop
    case AST_For: {
      astdef(For);

      auto _obj = this->evaluate(ast->iterable);
      _obj->ref_count++;

      auto& v = this->push_vst();

      auto& loop = this->loop_stack.emplace_front(v);

      Object** p_iter = nullptr;

      if (ast->iter->kind == AST_Variable) {
        p_iter = &v.append_lvar(nullptr);
      }
      else {
        p_iter = &this->eval_left(ast->iter);
      }

      switch (_obj->type.kind) {
        case TYPE_Range: {
          auto& iter = *(ObjLong**)p_iter;
          auto obj = (ObjRange*)_obj;

          iter = new ObjLong(obj->begin);
          iter->ref_count = 1;

          while (iter->value < obj->end) {
            this->evaluate(ast->code);

            if (loop.is_breaked) {
              break;
            }

            iter->value++;
            loop.is_continued = false;
          }

          // delete iter;
          iter->ref_count = 0;
          this->delete_object(iter);

          break;
        }

        default:
          todo_impl;
      }

      this->loop_stack.pop_front();
      this->pop_vst();

      _obj->ref_count--;
      break;
    }

    //
    // while
    case AST_While: {
      astdef(While);

      auto& vst = this->get_vst();
      auto& loop = this->loop_stack.emplace_front(vst);

      while (((ObjBool*)this->evaluate(ast->cond))->value) {
        this->evaluate(ast->code);

        if (loop.is_breaked)
          break;

        loop.is_continued = false;
      }

      this->loop_stack.pop_front();

      break;
    }

    //
    // do-while
    case AST_DoWhile: {
      astdef(DoWhile);

      auto& vst = this->get_vst();
      auto& loop = this->loop_stack.emplace_front(vst);

      do {
        this->evaluate(ast->code);

        if (loop.is_breaked)
          break;

        loop.is_continued = false;
      } while (
          ((ObjBool*)this->evaluate(ast->cond))->value);

      this->loop_stack.pop_front();

      break;
    }
  }

  return nullptr;
}
