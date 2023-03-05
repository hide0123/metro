#pragma once

namespace AST {

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

template <class Kind, ASTKind _self_kind>
struct ExprBase : Base {
  struct Element {
    Kind kind;
    Token const& op;
    Base* ast;

    explicit Element(Kind kind, Token const& op, Base* ast);
    ~Element();
  };

  Base* first;
  std::vector<Element> elements;

  ExprBase(Base* first);
  ~ExprBase();

  Element& append(Kind kind, Token const& op, Base* ast);

  std::string to_string() const;

  static ExprBase* create(Base*& ast);
};

}  // namespace AST