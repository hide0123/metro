#include "AST/AST.h"
#include "Utils.h"
#include "Debug.h"

namespace AST {

static int indent;
static bool add_ast_info;

static char const* kindlist[] = {
  [AST_Type] = "Type",
  [AST_None] = "None",
  [AST_True] = "True",
  [AST_False] = "False",
  [AST_Cast] = "Cast",
  [AST_Value] = "Value",
  [AST_Vector] = "Vector",
  [AST_Dict] = "Dict",
  [AST_Variable] = "Variable",
  [AST_MemberVariable] = "MemberVariable",
  [AST_CallFunc] = "CallFunc",
  [AST_NewEnumerator] = "NewEnumerator",
  [AST_StructConstructor] = "StructConstructor",
  [AST_IndexRef] = "IndexRef",
  [AST_MemberAccess] = "MemberAccess",
  [AST_UnaryPlus] = "UnaryPlus",
  [AST_UnaryMinus] = "UnaryMinus",
  [AST_Compare] = "Compare",
  [AST_Range] = "Range",
  [AST_Assign] = "Assign",
  [AST_Expr] = "Expr",
  [AST_If] = "If",
  [AST_Switch] = "Switch",
  [AST_Case] = "Case",
  [AST_Return] = "Return",
  [AST_Break] = "Break",
  [AST_Continue] = "Continue",
  [AST_Loop] = "Loop",
  [AST_For] = "For",
  [AST_While] = "While",
  [AST_DoWhile] = "DoWhile",
  [AST_Scope] = "Scope",
  [AST_Let] = "Let",
  [AST_Argument] = "Argument",
  [AST_Enum] = "Enum",
  [AST_Struct] = "Struct",
  [AST_Function] = "Function",
  [AST_Impl] = "Impl",
};

static std::string to_string(Base* _ast)
{
#define astdef(T) auto ast = (AST::T*)_ast

  static int colValue = 1;
  static std::list<std::string> colorstrlist;

  if (!_ast)
    return "(null)";

  std::string ret;
  std::string tabstr;

  auto color = Utils::format("\033[3%dm", colValue++);

  colorstrlist.push_front(color);

  if (colValue == 7)
    colValue = 1;

  for (int i = 0; i < indent; i++)
    tabstr += "  ";

  switch (_ast->kind) {
    case AST_None:
    case AST_Value:
    case AST_Variable:
    case AST_True:
    case AST_False:
      ret = _ast->token.str;
      break;

    case AST_CallFunc: {
      astdef(CallFunc);

      ret = std::string(ast->name) + "(" +
            Utils::String::join(", ", ast->args, to_string) + ")";

      break;
    }

    case AST_NewEnumerator: {
      astdef(NewEnumerator);

      ret = std::string(ast->ast_enum->name) + "." +
            std::string(ast->ast_enum->enumerators[ast->index].name);

      if (!ast->args.empty())
        ret += std::string(ast->name) + "(" +
               Utils::String::join(", ", ast->args, to_string) + ")";

      break;
    }

    case AST_StructConstructor: {
      astdef(StructConstructor);

      ret = "new " + to_string(ast->type) + "(" +
            Utils::String::join(", ", ast->init_pair_list,
                                [](StructConstructor::Pair& pair) {
                                  return std::string(pair.name) + ": " +
                                         to_string(pair.expr);
                                }) +
            ")";

      break;
    }

    case AST_IndexRef: {
      astdef(IndexRef);

      ret = to_string(ast->expr);

      for (auto&& index : ast->indexes) {
        switch (index.kind) {
          case IndexRef::Subscript::SUB_Index:
            ret += "[" + to_string(index.ast) + "]";
            break;

          case IndexRef::Subscript::SUB_Member:
          case IndexRef::Subscript::SUB_CallFunc:
            ret += "." + to_string(index.ast);
            break;
        }
      }

      break;
    }

    case AST_Vector: {
      astdef(Vector);

      ret = "[" + Utils::String::join(", ", ast->elements, to_string) + "]";

      break;
    }

    case AST_Range: {
      astdef(Range);

      ret = to_string(ast->begin) + " .. " + to_string(ast->end);

      break;
    }

    case AST_Expr: {
      astdef(Expr);

      ret = to_string(ast->first);

      for (auto&& elem : ast->elements) {
        ret += " " + std::string(elem.op.str) + " " + to_string(elem.ast);
      }

      break;
    }

    case AST_Compare: {
      astdef(Compare);

      ret = to_string(ast->first);

      for (auto&& elem : ast->elements) {
        ret += " " + std::string(elem.op.str) + " " + to_string(elem.ast);
      }

      break;
    }

    case AST_Assign: {
      astdef(Assign);

      ret = to_string(ast->dest) + " = " + to_string(ast->expr);

      break;
    }

    case AST_Let: {
      astdef(VariableDeclaration);

      std::string ret = "let " + std::string(ast->name);

      if (ast->type)
        ret += " : " + to_string(ast->type);

      if (ast->init)
        ret += " = " + to_string(ast->init);

      return ret;
    }

    case AST_Return: {
      astdef(Return);

      ret = "return";

      if (ast->expr)
        ret += " " + to_string(ast->expr);

      break;
    }

    case AST_Scope: {
      astdef(Scope);

      if (ast->is_empty()) {
        ret = "{ }";
        break;
      }

      indent++;

      ret = "{\n  " + tabstr;

      ret +=
        Utils::String::join("\n  " + tabstr, ast->list, [&ast](AST::Base* x) {
          auto s = to_string(x);

          if (*ast->list.rbegin() == x)
            return s;

          if (x->is_ended_with_scope())
            s += "\n";
          else
            s += ";";

          return s;
        });

      if (!ast->return_last_expr &&
          !(*ast->list.rbegin())->is_ended_with_scope())
        ret += ";";

      indent--;

      ret += "\n" + tabstr + "}";

      break;
    }

    case AST_If: {
      astdef(If);

      ret = "if " + to_string(ast->condition) + " " + to_string(ast->if_true);

      if (ast->if_false) {
        ret += "\n" + tabstr + "else " + to_string(ast->if_false);
      }

      break;
    }

    case AST_For: {
      astdef(For);

      ret = "for " + to_string(ast->iter) + " in " + to_string(ast->iterable) +
            " " + to_string(ast->code);

      break;
    }

    case AST_Argument: {
      astdef(Argument);

      ret = std::string(ast->name) + ": " + to_string(ast->type);

      break;
    }

    case AST_Function: {
      astdef(Function);

      ret = "fn " + std::string(ast->name.str) + "(";

      if (ast->have_self) {
        ret += "self";

        if (!ast->args.empty())
          ret += ", ";
      }

      ret += Utils::String::join(", ", ast->args, to_string) + ")";

      if (ast->result_type)
        ret += " -> " + to_string(ast->result_type) + " ";

      ret += to_string(ast->code);

      break;
    }

    case AST_Enum: {
      astdef(Enum);

      ret = "enum " + std::string(ast->name) + " {\n  " + tabstr;

      for (auto&& e : ast->enumerators) {
        ret += std::string(e.name);

        if (e.value_type)
          ret += "(" + to_string(e.value_type) + ")";

        if (&e != &*ast->enumerators.rbegin())
          ret += ",\n  " + tabstr;
      }

      ret += "\n" + tabstr + "}";

      break;
    }

    case AST_Struct: {
      astdef(Struct);

      ret = "struct " + std::string(ast->name) + " {\n  " + tabstr;

      for (auto&& m : ast->members) {
        ret += std::string(m.name) + ": " + to_string(m.type);

        if (&m != &*ast->members.rbegin())
          ret += ",\n  " + tabstr;
      }

      ret += "\n" + tabstr + "}";

      break;
    }

    case AST_Impl: {
      astdef(Impl);

      indent++;

      ret = "impl " + to_string(ast->type) + " {\n  " + tabstr +
            Utils::String::join("\n  " + tabstr, ast->list, to_string) + "\n" +
            tabstr + "}";

      indent--;

      break;
    }

    case AST_Type: {
      astdef(Type);

      ret = ast->name;

      if (!ast->parameters.empty()) {
        ret +=
          "<" + Utils::String::join(", ", ast->parameters, to_string) + ">";
      }

      if (ast->is_const)
        ret += " const";

      break;
    }

    default:
      alertmsg((int)_ast->kind);
      todo_impl;
  }

  if (add_ast_info) {
    ret = Utils::format("<AST_%s (%p)> ", kindlist[_ast->kind], _ast) + ret;
  }

  colorstrlist.pop_front();

  ret = color + ret;

  if (colorstrlist.empty())
    ret += color;
  else
    ret += *colorstrlist.begin();

  return ret;
}

std::string Base::to_string(Base* ast, bool _add_ast_info, bool _add_token_info)
{
  add_ast_info = _add_ast_info;
  return ::AST::to_string(ast) + COL_DEFAULT;
}

}  // namespace AST
