#pragma once

#include <list>
#include "AST.h"

// ---------------------------------------------
//  Parser
// ---------------------------------------------
class Parser {
  using token_iter = std::list<Token>::iterator;

public:
  Parser(std::list<Token>& token_list);
  ~Parser();

  AST::Scope* parse();

  AST::Base* primary();
  AST::Base* unary();
  AST::Base* indexref();
  AST::Base* member_access();

  AST::Base* mul();
  AST::Base* add();

  AST::Base* shift();
  AST::Base* compare();
  AST::Base* bit_op();

  AST::Base* range();

  AST::Base* assign();

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

  auto& to_return_stmt(AST::Base*& ast)
  {
    return ast = (AST::Base*)this->new_return_stmt(ast);
  }

  token_iter cur;
  token_iter ate;
};
