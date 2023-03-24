// ---------------------------------------------
//  Parser
// ---------------------------------------------

#pragma once

#include <list>
#include "ASTfwd.h"

class ScriptFileContext;
class Parser {
  using token_iter = std::list<Token>::iterator;

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
  token_iter next();

  bool eat(char const* s);
  bool found(char const* s);
  token_iter expect(char const* s);

  token_iter get(size_t offs);

  bool eat_semi();
  token_iter expect_semi();

  bool is_ended_with_scope(AST::Base* ast);

  token_iter expect_identifier();

  AST::Variable* new_variable();

  AST::Type* parse_typename();
  AST::Type* expect_typename();

  AST::Scope* parse_scope(token_iter tok = {}, AST::Base* first = nullptr);
  AST::Scope* expect_scope();

  AST::Function* parse_function();
  AST::Enum* parse_enum();
  AST::Struct* parse_struct();
  AST::Impl* parse_impl();

  // ast を return 文でラップする
  AST::Return* new_return_stmt(AST::Base* ast);

  AST::Base* set_last_token(AST::Base* ast)
  {
    ast->end_token = this->cur;
    ast->end_token--;

    return ast;
  }

  auto& to_return_stmt(AST::Base*& ast)
  {
    return ast = (AST::Base*)this->new_return_stmt(ast);
  }

  token_iter cur;
  token_iter ate;

  ScriptFileContext& _context;
  std::list<Token>& _token_list;
};
