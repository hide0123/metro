#pragma once

#include <string>
#include <vector>
#include <map>
#include "Token.h"
#include "ASTfwd.h"

// ---------------------------------------------
//  AST
// ---------------------------------------------

enum ASTKind : uint8_t {
  AST_Type,

  AST_None,
  AST_True,
  AST_False,

  AST_Cast,

  AST_Value,
  AST_Array,
  AST_Dict,

  AST_Variable,
  AST_GlobalVar,

  AST_IndexRef,
  AST_MemberAccess,

  AST_CallFunc,

  AST_UnaryPlus,
  AST_UnaryMinus,

  AST_Compare,

  AST_Range,

  AST_Assign,
  AST_Expr,

  AST_If,
  AST_Switch,
  AST_Return,

  AST_Loop,
  AST_For,
  AST_While,
  AST_DoWhile,

  AST_Break,
  AST_Continue,

  AST_Scope,

  AST_Let,
  AST_Shadow,

  AST_Function
};

namespace AST {

enum CmpKind {
  CMP_LeftBigger,  // >
  CMP_RightBigger,  // <
  CMP_LeftBigOrEqual,  // >=
  CMP_RightBigOrEqual,  // <=
  CMP_Equal,
  CMP_NotEqual,
};

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
  EX_BitOR,  // |

  EX_And,  // &&
  EX_Or,  // ||
};

struct Base {
  ASTKind kind;
  Token const& token;

  virtual ~Base()
  {
  }

  virtual std::string to_string() const;

protected:
  explicit Base(ASTKind kind, Token const& token)
      : kind(kind),
        token(token)
  {
  }
};

/**
 * @brief constant value keyword
 * example: none, true, false, ...
 *
 */
struct ConstKeyword : Base {
  ConstKeyword(ASTKind kind, Token const& token)
      : Base(kind, token)
  {
  }
};

struct Type : Base {
  std::vector<Type*> parameters;
  bool is_const;

  explicit Type(Token const& token)
      : Base(AST_Type, token),
        is_const(false)
  {
  }

  ~Type()
  {
    for (auto&& p : this->parameters)
      delete p;
  }
};

struct UnaryOp : Base {
  Base* expr;

  UnaryOp(ASTKind kind, Token const& token, Base* expr)
      : Base(kind, token),
        expr(expr)
  {
  }

  ~UnaryOp()
  {
    delete this->expr;
  }
};

struct Type;
struct Cast : Base {
  Type* cast_to;
  Base* expr;

  Cast(Token const& token)
      : Base(AST_Cast, token)
  {
  }

  ~Cast()
  {
    delete this->cast_to;
    delete this->expr;
  }
};

struct Value : Base {
  std::string to_string() const;

  Value(Token const& tok)
      : Base(AST_Value, tok)
  {
  }
};

struct Array : Base {
  std::vector<Base*> elements;

  Base*& append(Base* ast)
  {
    return this->elements.emplace_back(ast);
  }

  Array(Token const& token)
      : Base(AST_Array, token)
  {
  }

  ~Array()
  {
    for (auto&& e : this->elements)
      delete e;
  }
};

struct Dict : Base {
  struct Item {
    Token const& colon;
    Base* key;
    Base* value;

    Item(Token const& colon, Base* k, Base* v)
        : colon(colon),
          key(k),
          value(v)
    {
    }

    ~Item()
    {
      delete this->key;
      delete this->value;
    }
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

  ~Dict()
  {
    delete this->key_type;
    delete this->value_type;
  }
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

  ~IndexRef()
  {
    delete this->expr;

    for (auto&& i : this->indexes)
      delete i;
  }
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

  ~Range()
  {
    delete this->begin;
    delete this->end;
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

  ~CallFunc()
  {
    for (auto&& arg : this->args)
      delete arg;
  }
};

template <class Kind, ASTKind _self_kind>
struct ExprBase : Base {
  struct Element {
    Kind kind;
    Token const& op;
    Base* ast;

    explicit Element(Kind kind, Token const& op, Base* ast)
        : kind(kind),
          op(op),
          ast(ast)
    {
    }

    ~Element()
    {
      delete this->ast;
    }
  };

  Base* first;
  std::vector<Element> elements;

  std::string to_string() const
  {
    auto s = this->first->to_string();

    for (auto&& elem : this->elements) {
      s += " " + std::string(elem.op.str) + " " +
           elem.ast->to_string();
    }

    return s;
  }

  Element& append(Kind kind, Token const& op, Base* ast)
  {
    return this->elements.emplace_back(kind, op, ast);
  }

  static ExprBase* create(Base*& ast)
  {
    if (ast->kind != _self_kind)
      ast = new ExprBase<Kind, _self_kind>(ast);

    return (ExprBase*)ast;
  }

  ExprBase(Base* first)
      : Base(_self_kind, first->token),
        first(first)
  {
  }

  ~ExprBase()
  {
    delete this->first;
  }
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

  ~Assign()
  {
    delete this->dest;
    delete this->expr;
  }
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
};

struct Return : Base {
  AST::Base* expr;

  explicit Return(Token const& token)
      : Base(AST_Return, token),
        expr(nullptr)
  {
  }
};

struct LoopController : Base {
  LoopController(Token const& token, ASTKind kind)
      : Base(kind, token)
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
};

struct Scope;
struct While : Base {
  Base* cond;
  Scope* code;

  While(Token const& tok)
      : Base(AST_While, tok),
        cond(nullptr),
        code(nullptr)
  {
  }
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
};

struct Loop : Base {
  Base* code;

  Loop(Base* cc)
      : Base(AST_Loop, cc->token),
        code(cc)
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

  Token const& name;  // 名前
  std::vector<Argument> args;  // 引数

  AST::Type* result_type;  // 戻り値の型
  AST::Scope* code;  // 処理

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
};

}  // namespace AST
