#include "AST/AST.h"
#include "ASTWalker.h"
#include "Debug.h"

namespace ASTWalker {

bool walk(AST::Base* _ast, std::function<bool(AST::Base*)> func)
{
  if (!_ast)
    return true;

  if (!func(_ast))
    return false;

  switch (_ast->kind) {
    case AST_None:
    case AST_True:
    case AST_False:
    case AST_Value:
    case AST_Variable:
    case AST_NewEnumerator:
      break;

    case AST_Break:
    case AST_Continue:
      break;

    case AST_UnaryMinus:
    case AST_UnaryPlus:
      return walk(((AST::UnaryOp*)_ast)->expr, func);

    case AST_CallFunc: {
      astdef(CallFunc);

      for (auto&& x : ast->args) {
        if (!walk(x, func))
          return false;
      }

      break;
    }

    case AST_Cast: {
      astdef(Cast);

      return walk(ast->cast_to, func) && walk(ast->expr, func);
    }

    case AST_Vector: {
      astdef(Vector);

      for (auto&& e : ast->elements) {
        if (!walk(e, func))
          return false;
      }

      break;
    }

    case AST_StructConstructor: {
      astdef(StructConstructor);

      if (walk(ast->type, func)) {
        for (auto&& pair : ast->init_pair_list) {
          if (!walk(pair.expr, func))
            return false;
        }
      }

      break;
    }

    case AST_Dict: {
      astdef(Dict);

      if (walk(ast->key_type, func) && walk(ast->value_type, func)) {
        for (auto&& item : ast->elements) {
          if (!walk(item.key, func) || !walk(item.value, func))
            return false;
        }
      }

      break;
    }

    case AST_IndexRef: {
      astdef(IndexRef);

      if (walk(ast->expr, func)) {
        for (auto&& index : ast->indexes)
          if (!walk(index.ast, func))
            return false;
      }

      break;
    }

    case AST_Range: {
      astdef(Range);

      return walk(ast->begin, func) && walk(ast->end, func);
    }

    case AST_Expr: {
      astdef(Expr);

      if (walk(ast->first, func)) {
        for (auto&& e : ast->elements)
          if (!walk(e.ast, func))
            return false;
      }

      break;
    }

    case AST_Assign: {
      astdef(Assign);

      return walk(ast->dest, func) && walk(ast->expr, func);
    }

    case AST_Compare: {
      astdef(Compare);

      if (walk(ast->first, func)) {
        for (auto&& e : ast->elements)
          if (!walk(e.ast, func))
            return false;
      }

      break;
    }

    case AST_Let: {
      astdef(VariableDeclaration);

      return walk(ast->type, func) && walk(ast->init, func);
    }

    case AST_Return: {
      astdef(Return);
      return walk(ast->expr, func);
    }

    case AST_If: {
      astdef(If);
      return walk(ast->condition, func) && walk(ast->if_true, func) &&
             walk(ast->if_false, func);
    }

    case AST_Scope: {
      astdef(Scope);

      for (auto&& x : ast->list)
        if (!walk(x, func))
          return false;

      break;
    }

    case AST_Function: {
      astdef(Function);

      for (auto&& arg : ast->args)
        if (!walk(arg, func))
          return false;

      return walk(ast->result_type, func) && walk(ast->code, func);
    }

    case AST_Type: {
      astdef(Type);

      for (auto&& param : ast->parameters)
        if (!walk(param, func))
          return false;

      break;
    }

    default:
      alertmsg((int)_ast->kind);
      todo_impl;
  }

  return true;
}

}  // namespace ASTWalker