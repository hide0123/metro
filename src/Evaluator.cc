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

void Evaluator::gc_stop()
{
  _gc_stopped = false;
}

void Evaluator::gc_resume()
{
  _gc_stopped = true;
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

Object* Evaluator::default_constructor(TypeInfo const& type,
                                       bool construct_member)
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

    case TYPE_Vector: {
      auto ret = new ObjVector;

      ret->type = type;

      return ret;
    }

    case TYPE_UserDef: {
      auto ret = new ObjUserType(type.userdef_type);

      if (construct_member && type.userdef_type->kind == AST_Struct) {
        for (auto&& member : type.members) {
          ret->add_member(this->default_constructor(member.second));
        }
      }

      return ret;
    }
  }

  panic("u9r043290");
}

void Evaluator::eval_expr_elem(AST::Expr::Element const& elem, Object* dest)
{
  auto const& op = elem.op;

  auto right = this->evaluate(elem.ast);

  switch (elem.kind) {
    case AST::EX_Add: {
      switch (dest->type.kind) {
        case TYPE_Int:
          ((ObjLong*)dest)->value += ((ObjLong*)right)->value;
          break;

        case TYPE_String:
          ((ObjString*)dest)->append((ObjString*)right);
          break;

        default:
          todo_impl;
      }
      break;
    }

    case AST::EX_Sub: {
      switch (dest->type.kind) {
        case TYPE_Int:
          ((ObjLong*)dest)->value -= ((ObjLong*)right)->value;
          break;

        default:
          todo_impl;
      }
      break;
    }

    case AST::EX_Mul: {
      switch (dest->type.kind) {
        case TYPE_Int:
          ((ObjLong*)dest)->value *= ((ObjLong*)right)->value;
          break;

        case TYPE_Float:
          ((ObjFloat*)dest)->value *= ((ObjFloat*)right)->value;
          break;

        default:
          todo_impl;
      }
      break;
    }

    case AST::EX_Div: {
      switch (dest->type.kind) {
        case TYPE_Int: {
          auto rval = ((ObjLong*)right)->value;

          if (rval == 0) {
            Error(op, "division by zero").emit().exit();
          }

          ((ObjLong*)dest)->value /= rval;
          break;
        }

        case TYPE_Float: {
          auto rval = ((ObjFloat*)right)->value;

          if (rval == 0) {
            Error(op, "division by zero").emit().exit();
          }

          ((ObjFloat*)dest)->value /= rval;
          break;
        }

        default:
          todo_impl;
      }
      break;
    }

    case AST::EX_LShift:
      ((ObjLong*)dest)->value <<= ((ObjLong*)right)->value;
      break;

    case AST::EX_RShift:
      ((ObjLong*)dest)->value >>= ((ObjLong*)right)->value;
      break;

    case AST::EX_BitAND:
      ((ObjLong*)dest)->value &= ((ObjLong*)right)->value;
      break;

    case AST::EX_BitXOR:
      ((ObjLong*)dest)->value ^= ((ObjLong*)right)->value;
      break;

    case AST::EX_BitOR:
      ((ObjLong*)dest)->value |= ((ObjLong*)right)->value;
      break;

    case AST::EX_And: {
      ((ObjBool*)dest)->value =
        ((ObjBool*)dest)->value && ((ObjBool*)right)->value;
      break;
    }

    case AST::EX_Or: {
      ((ObjBool*)dest)->value =
        ((ObjBool*)dest)->value || ((ObjBool*)right)->value;
      break;
    }

    default:
      todo_impl;
  }
}

bool Evaluator::compute_compare(AST::CmpKind kind, Object* left, Object* right)
{
  float a = left->type.kind == TYPE_Int ? ((ObjLong*)left)->value
                                        : ((ObjFloat*)left)->value;

  float b = right->type.kind == TYPE_Int ? ((ObjLong*)right)->value
                                         : ((ObjFloat*)right)->value;

  switch (kind) {
    case AST::CMP_LeftBigger:
      return a > b;

    case AST::CMP_RightBigger:
      return a < b;

    case AST::CMP_LeftBigOrEqual:
      return a >= b;

    case AST::CMP_RightBigOrEqual:
      return a <= b;

    case AST::CMP_Equal:
      return a == b;

    case AST::CMP_NotEqual:
      return a != b;
  }

  return false;
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

    case TYPE_String: {
      auto ws = Utils::String::to_wstr(std::string(ast->token.str));

      // remove double quotation
      ws.erase(ws.begin());
      ws.pop_back();

      obj = new ObjString(std::move(ws));

      break;
    }

    default:
      debug(std::cout << type.to_string() << std::endl);

      todo_impl;
  }

  assert(obj != nullptr);

  obj->no_delete = 1;

  return obj;
}

Evaluator::FunctionStack& Evaluator::enter_function(AST::Function* func)
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

Object* Evaluator::evaluate(AST::Base* _ast)
{
  if (!_ast)
    return new ObjNone();

#if METRO_DEBUG
  if (!_ast->__checked) {
    Error(_ast, "@@@ didnt checked").emit();
  }
#endif

  if (_ast->use_default)
    return this->default_constructor(Sema::value_type_cache[_ast]);

  switch (_ast->kind) {
    case AST_None:
    case AST_Function:
    case AST_Enum:
    case AST_Struct:
    case AST_Impl:
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
          ((ObjFloat*)obj)->value = -((ObjFloat*)obj)->value;
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

      auto const& cast_to = Sema::value_type_cache[ast->cast_to];

      auto obj = this->evaluate(ast->expr);

      switch (cast_to.kind) {
        case TYPE_Int: {
          switch (obj->type.kind) {
            case TYPE_Float:
              return new ObjLong((float)((ObjFloat*)obj)->value);
          }
          break;
        }
      }

      todo_impl;
    }

    //
    // 即値
    case AST_Value: {
      alert;
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
    // 変数
    case AST_Variable:
      return this->eval_left(_ast)->clone();

    case AST_IndexRef: {
      astdef(IndexRef);

      if (ast->is_enum) {
        assert(ast->indexes.size() == 1);

        alert;
        auto ret = new ObjEnumerator(ast->enum_type, ast->enumerator_index);

        if (auto& x = ast->indexes[0];
            x.kind == AST::IndexRef::Subscript::SUB_CallFunc) {
          alert;
          ret->set_value(this->evaluate(((AST::CallFunc*)x.ast)->args[0]));
        }

        return ret;
      }

      auto obj = this->evaluate(ast->expr);

      if (ast->indexes.empty()) {
        alert;
        return obj;
      }

      return this->eval_index_ref(obj, ast);
    }

    //
    // Dictionary
    case AST_Dict: {
      astdef(Dict);

      auto ret = new ObjDict();
      ret->type = Sema::value_type_cache[_ast];

      for (auto&& elem : ast->elements) {
        ret->append(this->evaluate(elem.key), this->evaluate(elem.value));
      }

      return ret;
    }

    //
    // Range
    case AST_Range: {
      astdef(Range);

      auto begin = (ObjLong*)this->evaluate(ast->begin);
      auto end = (ObjLong*)this->evaluate(ast->end);

      return new ObjRange(begin->value, end->value);
    }

    //
    // 関数呼び出し
    case AST_CallFunc: {
      auto ast = (AST::CallFunc*)_ast;

      std::vector<Object*> args;

      // 引数
      for_indexed(i, arg, ast->args)
      {
        if (ast->is_membercall && arg->is_lvalue) {
          args.emplace_back(this->eval_left(arg));
          continue;
        }

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

    case AST_StructConstructor: {
      astdef(StructConstructor);

      alert;
      auto ret = new ObjUserType(ast->p_struct);

      for (auto&& pair : ast->init_pair_list) {
        alert;
        ret->add_member(this->evaluate(pair.expr));
      }

      alert;
      return ret;
    }

    //
    // 式
    case AST_Expr: {
      auto x = (AST::Expr*)_ast;

      alert;
      auto ret = this->evaluate(x->first)->clone();

      alert;
      ret->no_delete = true;

      for (auto&& elem : x->elements) {
        this->eval_expr_elem(elem, ret);
      }

      alert;
      ret->no_delete = false;

      return ret;
    }

    //
    // 代入
    case AST_Assign: {
      astdef(Assign);

      alert;
      auto& dest = this->eval_left(ast->dest);

      alert;
      dest->ref_count--;

      alert;
      dest = this->evaluate(ast->expr);
      alert;
      dest->ref_count++;

      alert;
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
        obj = this->evaluate(*iter++);

        if (item == last) {
          this->return_binds[obj] = ast;
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

      if (!ast->init || ast->ignore_initializer) {
        obj = this->default_constructor(Sema::value_type_cache[ast->type]);
      }
      else {
        obj = this->evaluate(ast->init);
      }

      obj->ref_count++;

      if (ast->is_shadowing) {
        auto& dest = this->get_vst().lvar_list[ast->index];

        dest->ref_count--;
        dest = obj;
      }
      else
        this->get_vst().append_lvar(obj);

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

      auto& v = this->push_vst();

      auto _obj = this->evaluate(ast->iterable);
      _obj->ref_count++;

      auto& loop = this->loop_stack.emplace_front(v);

      Object** p_iter = nullptr;

      if (ast->iter->kind == AST_Variable) {
        v.append_lvar(nullptr);
      }

      //      else {
      p_iter = &this->eval_left(ast->iter);
      //      }

      assert(p_iter);

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
          // this->delete_object(iter);

          break;
        }

        default:
          todo_impl;
      }

      this->pop_vst();
      this->clean_obj();

      this->loop_stack.pop_front();

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
      } while (((ObjBool*)this->evaluate(ast->cond))->value);

      this->loop_stack.pop_front();

      break;
    }

    default:
      alertmsg("evaluation is not implemented yet (kind=" << (int)_ast->kind
                                                          << ")");

      todo_impl;
  }

  return new ObjNone;
}

Object*& Evaluator::get_var(AST::Variable* ast)
{
  return std::next(this->vst_list.begin(), ast->step)->get_lvar(ast->index);
}

Object*& Evaluator::eval_left(AST::Base* _ast)
{
  switch (_ast->kind) {
    case AST_Variable:
      return this->get_var((AST::Variable*)_ast);

    case AST_IndexRef: {
      astdef(IndexRef);

      return this->eval_index_ref(this->eval_left(ast->expr), ast);
    }
  }

  throw 1;
}

Object*& Evaluator::eval_index_ref(Object*& obj, AST::IndexRef* ast)
{
  Object** ret = &obj;
  Object* tmp = nullptr;

  alert;
  for (auto&& index : ast->indexes) {
    alert;
    switch (index.kind) {
      case AST::IndexRef::Subscript::SUB_Index: {
        auto obj_index = this->evaluate(index.ast);

        switch ((*ret)->type.kind) {
          case TYPE_String: {
            auto& obj_str = *(ObjString**)ret;

            size_t indexval = 0;

            switch (obj_index->type.kind) {
              case TYPE_Int:
                indexval = ((ObjLong*)obj_index)->value;
                break;

              case TYPE_USize:
                indexval = ((ObjUSize*)obj_index)->value;
                break;

              default:
                panic("int or usize??aa");
            }

            if (indexval >= obj_str->characters.size()) {
              Error(index.ast, "index out of range").emit().exit();
            }

            ret = &(Object*&)obj_str->characters[indexval];
            break;
          }

          case TYPE_Vector: {
            auto& obj_vec = *(ObjVector**)ret;

            size_t indexval = 0;

            switch (obj_index->type.kind) {
              case TYPE_Int:
                indexval = ((ObjLong*)obj_index)->value;
                break;

              case TYPE_USize:
                indexval = ((ObjUSize*)obj_index)->value;
                break;

              default:
                panic("int or usize??aa");
            }

            if (indexval >= obj_vec->elements.size()) {
              Error(index.ast, "index out of range").emit().exit();
            }

            ret = &obj_vec->elements[indexval];
            break;
          }

          case TYPE_Dict: {
            auto& obj_dict = *(ObjDict**)ret;

            for (auto&& item : obj_dict->items) {
              if (item.key->equals(obj_index)) {
                ret = &item.value;
                alert;
                goto _dict_value_found;
              }
            }

            {
              alert;
              auto& item = obj_dict->append(
                obj_index,
                this->default_constructor(obj_dict->type.type_params[1]));

              alert;
              ret = &item.value;
            }

            alert;
          _dict_value_found:
            alert;
            break;
          }

          default:
            panic("no index");
        }

        break;
      }

      case AST::IndexRef::Subscript::SUB_Member: {
        alert;
        ret =
          &((ObjUserType*)*ret)->members[((AST::Variable*)index.ast)->index];

        break;
      }

      case AST::IndexRef::Subscript::SUB_CallFunc: {
        alert;
        auto cf_ast = (AST::CallFunc*)index.ast;
        auto func = cf_ast->callee;

        std::vector<Object*> args{*ret};

        for (auto&& arg : cf_ast->args) {
          args.emplace_back(this->evaluate(arg));
        }

        if (cf_ast->is_builtin) {
          todo_impl;
        }

        // コールスタック作成
        auto& cf = this->enter_function(func);

        // 引数
        auto& vst = this->push_vst();

        for (auto&& obj : args) {
          alert;
          vst.append_lvar(obj)->ref_count++;
        }

        // 関数実行
        alert;
        auto res = this->evaluate(func->code);

        // 戻り値を取得
        alert;
        auto result = cf.result;

        if (!cf.is_returned) {
          assert(func->code->return_last_expr);

          result = res;
        }

        assert(result != nullptr);

        for (auto&& obj : args) {
          obj->ref_count--;
        }

        this->pop_vst();

        // コールスタック削除
        this->leave_function();

        this->return_binds[result] = nullptr;

        alert;
        tmp = result;
        ret = &tmp;
        break;
      }

      default:
        todo_impl;
    }
  }

  return *ret;
}
