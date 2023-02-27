#pragma once

#include <string>
#include <vector>
#include "Token.h"

// ---------------------------------------------
//  AST
// ---------------------------------------------

enum ASTKind {
  AST_Type,

  AST_None,

  AST_Value,
  AST_Variable,
  AST_CallFunc,

  AST_Compare,

  AST_Assign,
  AST_Expr,

  AST_If,
  AST_Switch,
  AST_Return,

  AST_Loop,
  AST_For,
  AST_While,
  AST_DoWhile,

  AST_Scope,

  AST_Let,

  AST_Function
};

struct BuiltinFunc;

namespace AST {

struct Base {
  ASTKind kind;
  Token const& token;

  virtual ~Base() { }

  virtual std::string to_string() const;

protected:
  explicit Base(ASTKind kind, Token const& token)
    : kind(kind),
      token(token)
  {
  }
};

struct None : Base {
  None(Token const& token)
    : Base(AST_None, token)
  {
  }
};

struct Value : Base {
  Value(Token const& tok)
    : Base(AST_Value, tok)
  {
  }

  std::string to_string() const;
};

struct Variable : Base {
  size_t index;

  Variable(Token const& tok)
    : Base(AST_Variable, tok),
      index(0)
  {
  }
};

struct Function;
struct CallFunc : Base {
  std::string_view name;
  std::vector<Base*> args;

  bool is_builtin;
  BuiltinFunc const* builtin_func;
  Function* callee;

  std::string to_string() const;

  CallFunc(Token const& name)
    : Base(AST_CallFunc, name),
      name(name.str),
      is_builtin(false),
      builtin_func(nullptr),
      callee(nullptr)
  {
  }
};

struct Compare : Base {
  enum CmpKind {
    CMP_LeftBigger,       // >
    CMP_RightBigger,      // <
    CMP_LeftBigOrEqual,   // >=
    CMP_RightBigOrEqual,  // <=
    CMP_Equal,
    CMP_NotEqual,
  };

  struct Element {
    CmpKind kind;
    Token const& op;
    Base* ast;

    Element(CmpKind kind, Token const& op, Base* ast)
      : kind(kind),
        op(op),
        ast(ast)
    {
    }
  };

  Base* first;
  std::vector<Element> elements;

  Compare(Base* first)
    : Base(AST_Compare, first->token),
      first(first)
  {
  }

  Element& append(CmpKind kind, Token const& op, Base* ast) {
    return this->elements.emplace_back(kind, op, ast);
  }

  static Compare* create(Base*& ast) {
    if( ast->kind != AST_Compare )
      ast = new Compare(ast);

    return (Compare*)ast;
  }
};

struct Expr : Base {
  enum ExprKind {
    EX_Add,
    EX_Sub,
    EX_Mul,
    EX_Div,
    EX_Mod,
    
    EX_LShift,  // <<
    EX_RShift,  // >>

    EX_BitAND,  // &
    EX_BitXOR,  // ^
    EX_BitOR,   // |

    EX_And,   // &&
    EX_Or,    // ||
  };

  struct Element {
    ExprKind kind;
    Token const& op;
    Base* ast;

    explicit Element(ExprKind kind, Token const& op, Base* ast)
      : kind(kind),
        op(op),
        ast(ast)
    {
    }
  };

  Base* first;
  std::vector<Element> elements;

  Expr(Base* first)
    : Base(AST_Expr, first->token),
      first(first)
  {
  }

  std::string to_string() const;

  Element& append(ExprKind kind, Token const& op, Base* ast) {
    return this->elements.emplace_back(kind, op, ast);
  }

  static Expr* create(Base*& ast) {
    if( ast->kind != AST_Expr )
      ast = new Expr(ast);

    return (Expr*)ast;
  }
};

struct Assign : Base {
  Base* dest;
  Base* expr;

  Assign(Token const& assign_op)
  :Base(AST_Assign,assign_op)
  ,dest(nullptr),expr(nullptr)
  {}
};

//
// 変数定義
struct Type;
struct VariableDeclaration : Base {
  std::string_view name;
  Type* type;
  Base* init;

  explicit VariableDeclaration(Token const& token)
    : Base(AST_Let, token),
      type(nullptr),
      init(nullptr)
  {
  }
};

struct Return : Base {
  AST::Base* expr;

  explicit Return(Token const& token)
    : Base(AST_Return, token),
      expr(nullptr)
  {
  }
};

struct If : Base {
  AST::Base* condition;
  AST::Base* if_true;
  AST::Base* if_false;

  explicit If(Token const& token)
    : Base(AST_If, token),
      condition(nullptr),
      if_true(nullptr),
      if_false(nullptr)
  {
  }
};

struct For : Base {
  AST::Base* iter;
  AST::Base* iterable;

  For(Token const& tok)
    : Base(AST_For, tok)
      ,iter(nullptr),iterable(nullptr){}
};

struct While : Base {
  Base* cond;
  Scope* code;

  While(Token const&tok)
    :Base(AST_While,tok),
   cond(nullptr),code(nullptr){}
};

struct DoWhile : Base {
  Scope* code;
  Base* cond;

  DoWhile(Token const& tok)
    :Base(AST_DoWhile,tok)
    ,code(nullptr),cond(nullptr){}
};

struct Loop : Base {
  Base* code;

  Loop(Base* cc)
  :Base(AST_Loop,cc->token),
  code(cc)
  {
  }
};

struct Scope : Base {
  std::vector<Base*> list;
  size_t used_stack_size;

  Base*& append(Base* item) {
    return this->list.emplace_back(item);
  }

  explicit Scope(Token const& token)
    : Base(AST_Scope, token),
      used_stack_size(0)
  {
  }
};

struct Function : Base {
  struct Argument {
    Token const& name;
    AST::Type* type;

    explicit Argument(Token const& name, AST::Type* type)
      : name(name),
        type(type)
    {
    }
  };

  Token const& name;            // 名前
  std::vector<Argument> args;   // 引数

  AST::Type* result_type; // 戻り値の型
  AST::Scope* code;       // 処理

  size_t var_count = 0;

  /**
   * @brief 引数を追加する
   * 
   * @param name 
   * @param type 
   * @return Argument& 
   */
  Argument& append_argument(Token const& name, AST::Type* type) {
    return this->args.emplace_back(name, type);
  }

  /**
   * @brief Construct a new Function object
   * 
   * @param token 
   * @param name 
   */
  explicit Function(Token const& token, Token const& name)
    : Base(AST_Function, token),  
      name(name),
      result_type(nullptr),
      code(nullptr)
  {
  }
};

struct Type : Base {
  std::vector<Type*> parameters;
  bool is_mutable;

  explicit Type(Token const& token)
    : Base(AST_Type, token),
      is_mutable(false)
  {
  }
};



} // namespace AST
