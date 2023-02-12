#include "metro.h"

Parser::Parser(std::list<Token> const& token_list)
  : token_list(token_list),
    cur(token_list.begin()),
    ate(token_list.begin())
{
}

Parser::~Parser() {

}

AST::Base* Parser::parse() {
  auto root_scope = new AST::Scope(*this->cur);

  while( this->check() ) {
    root_scope->append(this->top());

    if( this->ate->str == "}" ) {
      continue;
    }
    else if( this->check() ) {
      this->expect_semi();
    }
  }

  return root_scope;
}

