#pragma once

#include <list>
#include <string>
#include <vector>

namespace AST {

struct Base {
  ASTKind kind;
  Token const& token;
  std::list<Token>::const_iterator end_token;

  virtual ~Base()
  {
  }

  virtual std::string to_string() const
  {
    return std::string(this->token.str);
  }

protected:
  explicit Base(ASTKind kind, Token const& token)
      : kind(kind),
        token(token),
        end_token(nullptr)
  {
  }
};

struct ListBase : Base {
  class ASTVector : public std::vector<Base*> {
  public:
    using std::vector<Base*>::vector;

    ~ASTVector()
    {
      for (auto&& x : *this)
        delete x;
    }
  };

  virtual bool is_empty() const = 0;
  virtual Base*& append(Base* ast) = 0;

protected:
  ListBase(ASTKind kind, Token const& token)
      : Base(kind, token)
  {
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

  std::string to_string() const
  {
    auto s = this->first->to_string();

    for (auto&& elem : this->elements) {
      s += " " + std::string(elem.op.str) + " " +
           elem.ast->to_string();
    }

    return s;
  }

  static ExprBase* create(Base*& ast)
  {
    if (ast->kind != _self_kind)
      ast = new ExprBase<Kind, _self_kind>(ast);

    return (ExprBase*)ast;
  }
};

}  // namespace AST