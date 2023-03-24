#pragma once

enum ASTKind : uint8_t {
  AST_Type,

  AST_UserTypeName,
  AST_ImplName,

  AST_None,
  AST_True,
  AST_False,

  AST_Cast,

  AST_Value,
  AST_Vector,
  AST_Dict,

  AST_Variable,
  AST_MemberVariable,
  AST_CallFunc,
  AST_TypeConstructor,

  AST_IndexRef,
  AST_MemberAccess,

  AST_UnaryPlus,
  AST_UnaryMinus,

  AST_Compare,

  AST_Range,

  AST_Assign,
  AST_Expr,

  //
  // control-statements
  AST_If,
  AST_Switch,
  AST_Case,
  AST_Return,
  AST_Break,
  AST_Continue,

  //
  // loop-statements
  AST_Loop,
  AST_For,
  AST_While,
  AST_DoWhile,

  //
  // A Scope
  AST_Scope,

  AST_Let,

  AST_Argument,

  AST_Enum,
  AST_Struct,
  AST_Function,

  AST_Impl,
};

namespace AST {

enum CmpKind : uint8_t {
  CMP_LeftBigger,  // >
  CMP_RightBigger,  // <
  CMP_LeftBigOrEqual,  // >=
  CMP_RightBigOrEqual,  // <=
  CMP_Equal,
  CMP_NotEqual,
};

enum ExprKind : uint8_t {
  EX_Add,
  EX_Sub,
  EX_Mul,
  EX_Div,
  EX_Mod,

  EX_LShift,  // <<
  EX_RShift,  // >>

  EX_BitAND,  // &
  EX_BitXOR,  // ^
  EX_BitOR,  // |

  EX_And,  // &&
  EX_Or,  // ||
};

}  // namespace AST