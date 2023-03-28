#pragma once

#include <list>
#include <string>
#include <vector>

namespace AST {

struct Base {
  ASTKind kind;
  Token const& token;
  std::list<Token>::const_iterator end_token;

  bool is_lvalue;

#if METRO_DEBUG
  bool __checked = false;
#endif

  bool use_default;  // empty scope or vector

  virtual ~Base();

  //
  // 空のスコープであるかどうか調べる
  bool is_empty_scope() const;

  //
  // 空の vector であるか
  bool is_empty_vector() const;

  static std::string to_string(Base* ast);

protected:
  Base(ASTKind kind, Token const& token);
};

//
struct Mutable : Base {
  ASTKind new_kind;

  ~Mutable()
  {
  }

protected:
  Mutable(ASTKind kind, Token const& token)
    : Base(kind, token),
      new_kind(kind)
  {
  }
};

struct ListBase : Base {
  class ASTVector : public std::vector<Base*> {
  public:
    using std::vector<Base*>::vector;

    ~ASTVector();
  };

  virtual bool is_empty() const = 0;
  virtual Base*& append(Base* ast) = 0;

protected:
  ListBase(ASTKind kind, Token const& token);
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
    }
  };

  Base* first;
  std::vector<Element> elements;

  ExprBase(Base* first)
    : Base(_self_kind, first->token),
      first(first)
  {
  }

  ~ExprBase()
  {
    delete this->first;

    for (auto&& elem : this->elements) {
      delete elem.ast;
    }
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
};

}  // namespace AST