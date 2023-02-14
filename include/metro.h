#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <functional>
#include <optional>

#define metro_debug 1

#if metro_debug
  #include <iostream>
  #include <cassert>

  #define debug(...) __VA_ARGS__
#else
  #define debug(...)
#endif

#define alert \
  fprintf(stderr,"\t#alert at %s:%d\n",__FILE__,__LINE__)

#define print_variable(fmt, v) \
  fprintf(stderr,"\t#viewvar: " #v " = " fmt "\n", v)

#define panic(fmt, ...) \
  fprintf(stderr, "\t#panic at %s:%d: " fmt "\n", \
    __FILE__, __LINE__, __VA_ARGS__), \
  exit(1)

#define alert_ctor \
  debug(fprintf("\t#alert_ctor ", ))

#define todo_impl panic("implement here %s", __func__)

#define TODO todo_impl

namespace Utils {
  
template <class ... Args>
std::string format(char const* fmt, Args&&... args) {
  static char buf[0x1000];
  sprintf(buf, fmt, args...);
  return buf;
}

namespace String {

std::wstring to_wstr(std::string const& str);
std::string to_str(std::wstring const& str);

} // namespace Utils::String

} // namespace Utils

// ---------------------------------------------
//  Token
// ---------------------------------------------

enum TokenKind {
  TOK_Int,
  TOK_Float,
  TOK_Char,
  TOK_String,
  TOK_Ident,
  TOK_Punctuater,
  TOK_End
};

struct Token {
  TokenKind kind;
  
  std::string_view str;

  size_t pos;

  static Token const semi;

  explicit Token(TokenKind kind)
    : kind(kind),
      pos(0)
  {
  }
};

// ---------------------------------------------
//  AST
// ---------------------------------------------

enum ASTKind {
  AST_Type,

  AST_None,

  AST_Value,
  AST_Variable,
  AST_CallFunc,

  AST_Expr,

  AST_If,
  AST_Switch,
  AST_Return,

  AST_Loop,
  AST_For,
  AST_While,

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

  virtual std::string to_string() const {
    return std::string(this->token.str);
  }

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

  CallFunc(Token const& name)
    : Base(AST_CallFunc, name),
      name(name.str),
      is_builtin(false),
      builtin_func(nullptr),
      callee(nullptr)
  {
  }
};

struct Expr : Base {
  enum ExprKind {
    EX_Add,
    EX_Sub,
    EX_Mul,
    EX_Div,

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

struct Scope : Base {
  std::vector<Base*> list;

  Base*& append(Base* item) {
    return this->list.emplace_back(item);
  }

  explicit Scope(Token const& token)
    : Base(AST_Scope, token)
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

  Token const& name;
  std::vector<Argument> args;

  AST::Type* result_type;
  AST::Scope* code;

  Argument& append_argument(Token const& name, AST::Type* type) {
    return this->args.emplace_back(name, type);
  }

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

// ---------------------------------------------
//  TypeInfo
// ---------------------------------------------
enum TypeKind {
  TYPE_None,
  TYPE_Int,
  TYPE_Float,
  TYPE_Char,
  TYPE_String,
  TYPE_Vector,
  TYPE_Args,
};

struct TypeInfo {
  TypeKind kind;
  bool is_mutable;

  std::vector<TypeInfo> type_params;

  TypeInfo(TypeKind kind = TYPE_None)
    : kind(kind),
      is_mutable(false)
  {
  }

  static std::vector<std::string> const& get_name_list();

  std::string to_string() const;

  bool equals(TypeInfo const& type) const;

  bool is_numeric() const;
};

// ---------------------------------------------
//  Object
// ---------------------------------------------

class Application;
struct ObjNone;
struct Object {
  TypeInfo type;
  size_t ref_count;

  static ObjNone* obj_none;

  static void initialize();

  virtual std::string to_string() const = 0;

  virtual Object* clone() const = 0;

  virtual ~Object();

protected:
  Object(TypeInfo type);

  friend class Application;
};

struct ObjNone : Object {
  ~ObjNone() { }

  std::string to_string() const;
  ObjNone* clone() const;

private:
  ObjNone()
    : Object(TYPE_None)
  {
  }

  friend struct Object;
};

struct ObjLong : Object {
  int64_t value;

  std::string to_string() const;
  ObjLong* clone() const;

  explicit ObjLong(int64_t value)
    : Object(TYPE_Int),
      value(value)
  {
  }
};

struct ObjFloat : Object {
  float value;

  std::string to_string() const;
  ObjFloat* clone() const;

  explicit ObjFloat(float value)
    : Object(TYPE_Float),
      value(value)
  {
  }
};

struct ObjString : Object {
  std::wstring value;

  std::string to_string() const;
  ObjString* clone() const;

  explicit ObjString(std::wstring value = L"")
    : Object(TYPE_String),
      value(value)
  {
  }
};

// ---------------------------------------------
//  BuiltinFunc
// ---------------------------------------------
struct BuiltinFunc {
  using Implementation =
    std::function<Object*(std::vector<Object*>&&)>;

  std::string name;     // 関数名

  TypeInfo result_type;             // 戻り値の型
  std::vector<TypeInfo> arg_types;  // 引数の型

  Implementation impl; // 処理

  // BuiltinFunc();

  static std::vector<BuiltinFunc> const& get_builtin_list();
};


// ---------------------------------------------
//  Lexer
// ---------------------------------------------
class Lexer {
public:
  Lexer(std::string const& source);
  ~Lexer();

  std::list<Token> lex();


private:
  bool check();
  char peek();
  bool match(std::string_view str);
  size_t pass_while(std::function<bool(char)> cond);
  
  size_t pass_space() {
    return this->pass_while([] (char c) { return isspace(c); });
  }

  std::string const& source;
  size_t position;
};


// ---------------------------------------------
//  Parser
// ---------------------------------------------
class Parser {
  using token_iter = std::list<Token>::const_iterator;

public:
  Parser(std::list<Token> const& token_list);
  ~Parser();


  AST::Scope* parse();

  AST::Base* primary();
  AST::Base* term();
  AST::Base* expr();

  AST::Base* top();

private:

  bool check();
  void next();

  bool eat(char const* s);
  token_iter expect(char const* s);

  bool eat_semi();
  token_iter expect_semi();

  token_iter expect_identifier();

  AST::Type* parse_typename();
  AST::Type* expect_typename();

  AST::Scope* parse_scope();
  AST::Scope* expect_scope();

  AST::Function* parse_function();

  // 引数 ast を、return 文でラップする
  AST::Return* new_return_stmt(AST::Base* ast);

  auto& to_return_stmt(AST::Base*& ast) {
    return ast = (AST::Base*)this->new_return_stmt(ast);
  }

  std::list<Token> const& token_list;

  token_iter cur;
  token_iter ate;
};

// ---------------------------------------------
//  Checker
// ---------------------------------------------
class Evaluator;
class Checker {
  friend class Evaluator;

public:

  Checker(AST::Scope* root);
  ~Checker();

  TypeInfo check(AST::Base* ast);
  
  // 関数呼び出しが正しいか検査する
  TypeInfo check_function_call(AST::CallFunc* ast);

  // 式の両辺の型が正しいかどうか検査する
  std::optional<TypeInfo> is_valid_expr(
    AST::Expr::ExprKind kind, TypeInfo const& lhs, TypeInfo const& rhs);

  AST::Function* find_function(std::string_view name);

  static void initialize();

private:

  AST::Scope* root;

  static std::map<AST::Value*, TypeInfo> value_type_cache;

};

// ---------------------------------------------
//  Evaluator
// ---------------------------------------------
class GarbageCollector;
class Evaluator {

  struct FunctionStack {
    AST::Function const* ast;

    Object* result;

    explicit FunctionStack(AST::Function const* ast)
      : ast(ast),
        result(nullptr)
    {
    }
  };

public:
  explicit Evaluator(GarbageCollector const&);
  ~Evaluator();


  Object* evaluate(AST::Base* ast);

  Object* add_object(Object* left, Object* right);
  Object* sub_object(Object* left, Object* right);
  Object* mul_object(Object* left, Object* right);
  Object* div_object(Object* left, Object* right);

private:

  Object* create_object(AST::Value* ast);

  FunctionStack& enter_function(AST::Function* func);
  void leave_function(AST::Function* func);

  FunctionStack& get_current_func_stack();

  std::vector<Object*> variable_stack;
  std::vector<FunctionStack> call_stack;

  std::map<AST::Value*, Object*> immediate_objects;

  GarbageCollector const& gc;
};

// ---------------------------------------------
//  Garbage Collector
// ---------------------------------------------
class GarbageCollector {
public:

  GarbageCollector();
  ~GarbageCollector();

  void register_object(Object* obj);


private:


};


// ---------------------------------------------
//  Error
// ---------------------------------------------

class Error {
public:
  struct ErrLoc {
    enum LocationType {
      LOC_Position,
      LOC_Token,
      LOC_AST
    };

    LocationType type;

    union {
      size_t pos;
      Token const* token;
      AST::Base const* ast;
    };

    // todo
    // ErrLoc(size_t pos)

    ErrLoc(Token const& token)
      : type(LOC_Token),
        token(&token)
    {
    }

    ErrLoc(AST::Base const* ast)
      : type(LOC_AST),
        ast(ast)
    {
    }
  };

  Error(ErrLoc loc, std::string const& msg)
    : loc(loc),
      msg(msg)
  {
  }

  Error& emit();

  [[noreturn]]
  void exit(int code = 1);


private:

  ErrLoc loc;
  std::string msg;
};



class Application {

public:
  Application();
  ~Application();

  void initialize();



private:


};

