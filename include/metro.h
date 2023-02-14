#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <list>
#include <tuple>
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
  debug(fprintf(stderr,"\t#alert_ctor at %s:%d: %s\n",__FILE__,__LINE__,\
  __func__))

#define alert_dtor \
  debug(fprintf(stderr,"\t#alert_dtor at %s:%d: %s\n",__FILE__,__LINE__,\
  __func__))

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
  std::string to_string() const;
  ObjNone* clone() const;

  explicit ObjNone()
    : Object(TYPE_None)
  {
  }
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
    std::function<Object*(std::vector<Object*> const&)>;

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

  AST::Base* stmt();

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

  struct FunctionContext {
    AST::Function* func;

    TypeInfo result_type;
  };

  struct VariableEmu {
    std::string_view name;

    TypeInfo type;
  };

  struct ScopeEmu {
    AST::Scope* ast;

    std::vector<VariableEmu> variables;

    int find_var(std::string_view name) {
      for( int i = 0; auto&& var : this->variables ) {
        if( var.name == name )
          return i;

        i++;
      }

      return -1;
    }
  };

public:

  Checker(AST::Scope* root);
  ~Checker();


  /**
   * @brief 構文木の意味解析、型一致確認など行う
   * 
   * 
   * @param _ast 
   * @return 評価された _ast の型 (TypeInfo)
   */
  TypeInfo check(AST::Base* ast);
  

  /**
   * @brief 関数呼び出しが正しいか検査する
   * 
   * @param ast 
   * @return TypeInfo 
   */
  TypeInfo check_function_call(AST::CallFunc* ast);


  /**
   * @brief スコープ
   * 
   * @param ast 
   */
  void enter_scope(AST::Scope* ast);
  

  /**
   * @brief  式の両辺の型が正しいかどうか検査する
   * 
   * @param kind 
   * @param lhs 
   * @param rhs 
   * @return std::optional<TypeInfo> 
   */
  std::optional<TypeInfo> is_valid_expr(
    AST::Expr::ExprKind kind, TypeInfo const& lhs, TypeInfo const& rhs);


  /**
   * @brief ユーザー定義関数を探す
   * 
   * @param name 
   * @return AST::Function* 
   */
  AST::Function* find_function(std::string_view name);


  /**
   * @brief 変数を探す
   * 
   * @param name 
   * @return std::optional<
   * std::tuple<std::list<ScopeEmu>::iterator, size_t, size_t>
   * > 
   */
  std::optional<
    std::tuple<
      std::list<ScopeEmu>::iterator,  // iter to ScopeEmu
      size_t, // in Evaluator, index to object n stack
      size_t  // index in ScopeEmu
    >
  >
    find_variable(std::string_view name);


private:

  ScopeEmu& get_cur_scope();

  AST::Scope* root;

  int call_count;

  std::list<ScopeEmu> scope_list;

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
  explicit Evaluator(GarbageCollector& gc);
  ~Evaluator();

  /**
   * @brief 構文木を評価する（実行）
   * 
   * @param ast 
   * @return Object* 
   */
  Object* evaluate(AST::Base* ast);


  //
  // 演算子
  static Object* compute_expr_operator(
    Token const& op_token,
    AST::Expr::ExprKind kind,
    Object* left,
    Object* right
  );


private:

  /**
   * @brief 即値・リテラルの構文木からオブジェクトを作成する
   * 
   * @note すでに作成済みであればそれを返す
   * 
   * @param ast 
   * @return Object*
   */
  Object* create_object(AST::Value* ast);


  /**
   * @brief 
   * 
   * @param func 
   * @return FunctionStack& 
   */
  FunctionStack& enter_function(AST::Function* func);


  /**
   * @brief 
   * 
   * @param func 
   */
  void leave_function(AST::Function* func);


  /**
   * @brief 現在のコールスタックを取得する
   * 
   * @return FunctionStack& 
   */
  FunctionStack& get_current_func_stack();


  /**
   * @brief オブジェクトをスタックに追加する
   * 
   * @param obj 
   * @return Object*& (追加されたオブジェクトへの参照)
   */
  Object*& push_object(Object* obj);


  /**
   * @brief スタックからオブジェクトを１個削除
   * 
   * @return Object* (スタックから削除されたオブジェクト)
   */
  Object* pop_object();


  /**
   * @brief スタックから指定された数だけオブジェクトを削除する
   */
  void pop_object_with_count(size_t count);


  //
  // オブジェクトスタック
  // 変数・引数で使う
  std::vector<Object*> object_stack;

  //
  // コールスタック
  // 関数呼び出し用
  std::vector<FunctionStack> call_stack;

  //
  // 即値・リテラル
  std::map<AST::Value*, Object*> immediate_objects;

  //
  // ガベージコレクタ
  GarbageCollector& gc;
};

// ---------------------------------------------
//  Garbage Collector
// ---------------------------------------------
class GarbageCollector {
public:

  GarbageCollector();
  ~GarbageCollector();

  void register_object(Object* obj);

  void clean();


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

