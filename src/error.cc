#include <iostream>

#include "AST.h"

#include "Error.h"

Error::ErrLoc::ErrLoc(size_t pos)
  : type(LOC_Position),
    pos(pos)
{
}

Error::ErrLoc::ErrLoc(Token const& token)
  : type(LOC_Token),
    token(&token)
{
}

Error::ErrLoc::ErrLoc(AST::Base const* ast)
  : type(LOC_AST),
    ast(ast)
{
}

Error& Error::emit() {
  std::cerr << this->msg << std::endl;

  return *this;
}

void Error::exit(int code) {
  std::exit(code);
}