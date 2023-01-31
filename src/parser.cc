#include "lcc.h"

Parser::Parser(std::list<Token> const& token_list)
  : token_list(token_list)
{
}

Parser::~Parser() {

}

AST::Base* Parser::parse() {
  return this->expr();
}

AST::Base* Parser::primary() {
  switch( this->cur->kind ) {

  }

  
}

AST::Base* Parser::term() {

}

AST::Base* Parser::expr() {

}

bool Parser::check() {
  return this->cur->kind != TOK_End;
}

void Parser::next() {
  this->cur++;
}

bool Parser::eat(char const* s) {
  if( this->cur->str == s ) {
    this->ate = this->cur;
    this->next();
    return true;
  }

  return false;
}