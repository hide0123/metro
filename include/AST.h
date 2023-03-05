// ---------------------------------------------
//  AST
// ---------------------------------------------
#pragma once

#include <map>
#include <vector>

#include "Token.h"
#include "ASTfwd.h"

#include "AST/Kind.h"
#include "AST/Base.h"

namespace AST {

struct Type : Base {
  std::vector<Type*> parameters;
  bool is_const;

  explicit Type(Token const& token)
      : Base(AST_Type, token),
        is_const(false)
  {
  }

  ~Type();
};

struct ConstKeyword : Base {
  ConstKeyword(ASTKind kind, Token const& token)
      : Base(kind, token)
  {
  }
};

struct UnaryOp : Base {
  Base* expr;

  UnaryOp(ASTKind kind, Token const& token, Base* expr)
      : Base(kind, token),
        expr(expr)
  {
  }

  ~UnaryOp();
};

struct Type;
struct Cast : Base {
  Type* cast_to;
  Base* expr;

  Cast(Token const& token)
      : Base(AST_Cast, token)
  {
  }

  ~Cast();
};

struct Value : Base {
  std::string to_string() const;

  Value(Token const& tok)
      : Base(AST_Value, tok)
  {
  }
};

struct Vector : Base {
  std::vector<Base*> elements;

  Base*& append(Base* ast)
  {
    return this->elements.emplace_back(ast);
  }

  Vector(Token const& token)
      : Base(AST_Vector, token)
  {
  }

  ~Vector();
};

struct Dict : Base {
  struct Item {
    Token const& colon;
    Base* key;
    Base* value;

    Item(Token const& colon, Base* k, Base* v);
    ~Item();
  };

  std::vector<Item> elements;

  Type* key_type;
  Type* value_type;

  Dict(Token const& token)
      : Base(AST_Dict, token),
        key_type(nullptr),
        value_type(nullptr)
  {
  }

  ~Dict();
};

struct Variable : Base {
  size_t index;

  Variable(Token const& tok)
      : Base(AST_Variable, tok),
        index(0)
  {
  }
};

struct IndexRef : Base {
  Base* expr;
  std::vector<Base*> indexes;

  IndexRef(Token const& t)
      : Base(AST_IndexRef, t),
        expr(nullptr)
  {
  }

  ~IndexRef();
};

struct Range : Base {
  Base* begin;
  Base* end;

  Range(Token const& token)
      : Base(AST_Range, token),
        begin(nullptr),
        end(nullptr)
  {
  }

  ~Range();
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

  ~CallFunc();
};

struct Assign : Base {
  Base* dest;
  Base* expr;

  Assign(Token const& assign_op)
      : Base(AST_Assign, assign_op),
        dest(nullptr),
        expr(nullptr)
  {
  }

  ~Assign();
};

//
// 変数定義
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

  ~VariableDeclaration();
};

struct Return : Base {
  AST::Base* expr;

  explicit Return(Token const& token)
      : Base(AST_Return, token),
        expr(nullptr)
  {
  }

  ~Return();
};

struct LoopController : Base {
  LoopController(Token const& token, ASTKind kind)
      : Base(kind, token)
  {
  }
};

struct Scope : Base {
  std::vector<Base*> list;
  size_t var_count;

  Base*& append(Base* item)
  {
    return this->list.emplace_back(item);
  }

  explicit Scope(Token const& token)
      : Base(AST_Scope, token),
        var_count(0)
  {
  }

  ~Scope();
};

struct If : Base {
  Base* condition;
  Base* if_true;
  Base* if_false;

  explicit If(Token const& token)
      : Base(AST_If, token),
        condition(nullptr),
        if_true(nullptr),
        if_false(nullptr)
  {
  }

  ~If();
};

struct For : Base {
  Base* iter;
  Base* iterable;
  Base* code;

  For(Token const& tok)
      : Base(AST_For, tok),
        iter(nullptr),
        iterable(nullptr),
        code(nullptr)
  {
  }

  ~For();
};

struct While : Base {
  Base* cond;
  Scope* code;

  While(Token const& tok)
      : Base(AST_While, tok),
        cond(nullptr),
        code(nullptr)
  {
  }

  ~While();
};

struct DoWhile : Base {
  Scope* code;
  Base* cond;

  DoWhile(Token const& tok)
      : Base(AST_DoWhile, tok),
        code(nullptr),
        cond(nullptr)
  {
  }

  ~DoWhile();
};

struct Loop : Base {
  Base* code;

  Loop(Base* code)
      : Base(AST_Loop, code->token),
        code(code)
  {
  }

  ~Loop();
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

    ~Argument()
    {
      delete this->type;
    }
  };

  Token const& name;  // 名前
  std::vector<Argument> args;  // 引数

  Type* result_type;  // 戻り値の型
  Scope* code;  // 処理

  size_t var_count = 0;

  /**
   * @brief 引数を追加する
   *
   * @param name
   * @param type
   * @return Argument&
   */
  Argument& append_argument(Token const& name, AST::Type* type)
  {
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

  ~Function();
};

using Expr = ExprBase<ExprKind, AST_Expr>;
using Compare = ExprBase<CmpKind, AST_Compare>;

}  // namespace AST
