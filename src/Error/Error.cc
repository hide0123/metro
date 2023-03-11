#include <iostream>
#include <cassert>

#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Error.h"
#include "ScriptFileContext.h"
#include "Application.h"

size_t Error::_count;

ErrorLocation::ErrorLocation(Token const& token)
    : loc_kind(ERRLOC_Token),
      ast(nullptr),
      token(&token),
      _line_num(token.src_loc.line_num)
{
}

ErrorLocation::ErrorLocation(AST::Base const* ast)
    : loc_kind(ERRLOC_AST),
      ast(ast),
      token(nullptr),
      _line_num(ast->token.src_loc.line_num)
{
  assert(&*ast->end_token);
}

Error::Error(ErrorKind kind, ErrorLocation&& loc,
             std::string const& msg)
    : _kind(kind),
      _loc(std::move(loc)),
      _is_single_line(false),
      _msg(msg),
      _pContext(
          Application::get_instance()->get_current_context())
{
}

Error::Error(ErrorLocation&& loc, std::string const& msg)
    : Error(ERR_None, std::move(loc), msg)
{
}

Error& Error::single_line()
{
  this->_is_single_line = true;
  return *this;
}

void Error::exit(int code)
{
  std::exit(code);
}

bool Error::was_emitted()
{
  return _count != 0;
}