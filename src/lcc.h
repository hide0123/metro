#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <functional>

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

} // namespace Utils

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

  explicit Token(TokenKind kind)
    : kind(kind),
      pos(0)
  {
  }

};

enum ASTKind {
  AST_Value,
  AST_Variable,

  AST_Mul,
  AST_Div,

  AST_Add,
  AST_Sub,

  AST_Expr,

  AST_Function
};

namespace AST {

struct Base {
  ASTKind kind;
  Token const& token;

  virtual ~Base() { }

protected:
  Base(ASTKind kind, Token const& token)
    : kind(kind),
      token(token)
  {
  }
};

struct Value : Base {
  Value(Token const& tok)
    : Base(AST_Value, tok)
  {
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

struct Expr : Base {
  struct Element {
    ASTKind kind;
    Base* ast;

    explicit Element(ASTKind kind, Base* ast)
      : kind(kind),
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

  Element& append(ASTKind kind, Base* ast) {
    return this->elements.emplace_back(kind, ast);
  }

  static Expr* create(Base* ast) {
    if( ast->kind != AST_Expr )
      return new Expr(ast);

    return (Expr*)ast;
  }
};

} // namespace AST

enum TypeKind {
  TYPE_None,
  TYPE_Int,
  TYPE_Float,
  TYPE_Char,
  TYPE_String,
  TYPE_Vector
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
};

struct Object {
  TypeInfo type;
  size_t ref_count;

  virtual ~Object() { }

protected:
  Object(TypeInfo type)
    : type(type),
      ref_count(0)
  {
  }
};

struct ObjLong : Object {
  int64_t value;

  ObjLong(int64_t value)
    : Object(TYPE_Int),
      value(value)
  {
  }
};

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
  

  std::list<Token> const& token_list;

  token_iter cur;
  token_iter ate;
};

class TypeChecker {
public:

  TypeInfo check(AST::Base* ast);
  
  bool is_valid_expr(ASTKind kind, TypeInfo lhs, TypeInfo rhs);

private:

};

class Evaluator {
public:
  Evaluator();
  ~Evaluator();


  Object* eval(AST::Base* ast);


private:


  std::map<AST::Value*, Object> immediate_objects;

};



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

    // todo
    // ErrLoc(AST::Base* pos)
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

