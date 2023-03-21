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

Object* Evaluator::default_constructor(TypeInfo const& type)
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

    case TYPE_UserDef:
      return new ObjUserType(type);
  }

  panic("u9r043290");
}

void Evaluator::eval_expr_elem(
    AST::Expr::Element const& elem, Object* dest)
{
  auto const& op = elem.op;

  auto right = this->evaluate(elem.ast);

  switch (elem.kind) {
    case AST::EX_Add: {
      switch (dest->type.kind) {
        case TYPE_Int:
          ((ObjLong*)dest)->value +=
              ((ObjLong*)right)->value;
          break;

        case TYPE_String:
          ((ObjString*)dest)->value +=
              ((ObjString*)right)->value;
          break;

        default:
          todo_impl;
      }
      break;
    }

    case AST::EX_Sub: {
      switch (dest->type.kind) {
        case TYPE_Int:
          ((ObjLong*)dest)->value -=
              ((ObjLong*)right)->value;
          break;

        default:
          todo_impl;
      }
      break;
    }

    case AST::EX_Mul: {
      switch (dest->type.kind) {
        case TYPE_Int:
          ((ObjLong*)dest)->value *=
              ((ObjLong*)right)->value;
          break;

        case TYPE_Float:
          ((ObjFloat*)dest)->value *=
              ((ObjFloat*)right)->value;
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
      ((ObjBool*)dest)->value = ((ObjBool*)dest)->value &&
                                ((ObjBool*)right)->value;
      break;
    }

    case AST::EX_Or: {
      ((ObjBool*)dest)->value = ((ObjBool*)dest)->value ||
                                ((ObjBool*)right)->value;
      break;
    }

    default:
      todo_impl;
  }
}

bool Evaluator::compute_compare(AST::CmpKind kind,
                                Object* left, Object* right)
{
  float a = left->type.kind == TYPE_Int
                ? ((ObjLong*)left)->value
                : ((ObjFloat*)left)->value;

  float b = right->type.kind == TYPE_Int
                ? ((ObjLong*)right)->value
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
      auto ws = Utils::String::to_wstr(
          std::string(ast->token.str));

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

Evaluator::FunctionStack& Evaluator::enter_function(
    AST::Function* func)
{
  return this->call_stack.emplace_front(func);
}

void Evaluator::leave_function()
{
  this->call_stack.pop_front();
}

Evaluator::FunctionStack&
Evaluator::get_current_func_stack()
{
  return *this->call_stack.begin();
}
