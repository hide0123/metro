// ---------------------------------------------
//  Parser
// ---------------------------------------------

#pragma once

#include <list>
#include <functional>
#include "AST/ASTfwd.h"

class ScriptFileContext;
class Parser {
  using TokenIterator = std::list<Token>::iterator;

public:
  Parser(ScriptFileContext& context, std::list<Token>& token_list);

  ~Parser();

  AST::Scope* parse();

  AST::Base* factor();
  AST::Base* primary();
  AST::Base* indexref();
  AST::Base* unary();
  AST::Base* mul();
  AST::Base* add();
  AST::Base* shift();
  AST::Base* compare();
  AST::Base* bit_op();
  AST::Base* log_and_or();
  AST::Base* range();
  AST::Base* assign();

  AST::Base* expr();
  AST::Base* stmt();

  AST::Base* top();

private:
  bool check();
  TokenIterator next();

  bool eat(char const* s);
  bool found(char const* s);
  TokenIterator expect(char const* s);

  TokenIterator get(size_t offs);

  bool eat_semi();
  TokenIterator expect_semi();

  TokenIterator expect_identifier();

  AST::Variable* new_variable();

  AST::Type* expect_typename();

  AST::Scope* parse_scope(TokenIterator tok = {}, AST::Base* first = nullptr);
  AST::Scope* expect_scope();

  AST::Function* parse_function();

  AST::Enum* parse_enum();
  AST::Struct* parse_struct();
  AST::Impl* parse_impl();

  // ast を return 文でラップする
  AST::Return* new_return_stmt(AST::Base* ast);

  AST::Base* set_last_token(AST::Base* ast);

  template <class... Args>
  bool token_match(Args&&... args);  // Parser.cc

  bool in_impl;

  TokenIterator cur;
  TokenIterator ate;

  ScriptFileContext& _context;
  std::list<Token>& _token_list;
};
