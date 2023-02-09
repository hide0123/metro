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

  #define debug(...) __VA_ARGS__
#else
  #define debug(...)
#endif

#define alert \
  fprintf(stderr,"\t#alert at %s:%d\n",__FILE__,__LINE__)

#define viewvar(fmt, v) \
  fprintf(stderr,"\t#viewvar: " #v " = " fmt "\n", v)

#define panic(fmt, ...) \
  fprintf(stderr, "\t#panic at %s:%d: " fmt "\n", \
    __FILE__, __LINE__, __VA_ARGS__), \
  exit(1)

#define todo_impl panic("implement here %s", __func__)

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
  AST_None,

  AST_Value,
  AST_Variable,
  AST_CallFunc,

  AST_If,
  AST_Loop,

  AST_Expr,

  AST_For,
  AST_While,
  AST_Switch,

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

struct CallFunc : Base {
  std::string_view name;
  std::vector<Base*> args;

  bool is_builtin;
  BuiltinFunc const* builtin_func;

  CallFunc(Token const& name)
    : Base(AST_CallFunc, name),
      name(name.str),
      is_builtin(false),
      builtin_func(nullptr)
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

  virtual ~Object() { }

protected:
  Object(TypeInfo type)
    : type(type),
      ref_count(0)
  {
  }

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

  friend class Object;
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
      value(0)
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

class Parser {
  using token_iter = std::list<Token>::const_iterator;

public:
  Parser(std::list<Token> const& token_list);
  ~Parser();


  AST::Base* parse();

  AST::Base* primary();
  AST::Base* term();
  AST::Base* expr();


private:

  bool check();
  void next();

  bool eat(char const* s);
  void expect(char const* s);
  

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

  TypeInfo check(AST::Base* ast);
  
  // 関数呼び出しが正しいか検査する
  void check_function_call(AST::CallFunc* ast);

  // 式の両辺の型が正しいかどうか検査する
  std::optional<TypeInfo> is_valid_expr(
    AST::Expr::ExprKind kind, TypeInfo const& lhs, TypeInfo const& rhs);

  static void initialize();

private:

  static std::map<AST::Value*, TypeInfo> value_type_cache;

};

// ---------------------------------------------
//  Evaluator
// ---------------------------------------------

class Evaluator {
public:
  Evaluator();
  ~Evaluator();


  Object* evaluate(AST::Base* ast);

  Object* add_object(Object* left, Object* right);
  Object* sub_object(Object* left, Object* right);
  Object* mul_object(Object* left, Object* right);
  Object* div_object(Object* left, Object* right);

private:

  Object* create_object(AST::Value* ast);

  std::map<AST::Value*, Object*> immediate_objects;
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

