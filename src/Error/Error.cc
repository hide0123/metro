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
      _pos_begin(token.src_loc.position),
      _pos_end(token.src_loc.get_end_pos())
{
}

ErrorLocation::ErrorLocation(AST::Base const* ast)
    : loc_kind(ERRLOC_AST),
      ast(ast),
      token(nullptr),
      _pos_begin(ast->token.src_loc.position),
      _pos_end(ast->end_token->src_loc.get_end_pos())
{
  assert(&*ast->end_token);
}

Error::Error(ErrorLocation&& loc, std::string const& msg)
    : _kind(ERR_None),
      _msg(msg),
      _loc(std::move(loc)),
      _pContext(nullptr),
      _is_single_line(false)
{
}

Error::Error(ErrorKind kind, ErrorLocation&& loc,
             std::string const& msg)
    : _kind(kind),
      _msg(msg),
      _loc(std::move(loc)),
      _pContext(nullptr),
      _is_single_line(false)

{
}

Error& Error::single_line()
{
  this->_is_single_line = true;
  return *this;
}

std::pair<Token const*, Token const*> Error::get_token_range()
    const
{
  Token const* begin = nullptr;
  Token const* end = nullptr;

  // ソースコード上の位置を取得
  switch (this->_loc.loc_kind) {
    case ERRLOC_AST: {
      begin = &this->_loc.ast->token;
      end = &*this->_loc.ast->end_token;
      break;
    }

    case ERRLOC_Token:
      begin = end = this->_loc.token;
      break;
  }

  assert(begin);
  assert(end);

  return {begin, end};
}

void Error::exit(int code)
{
  std::exit(code);
}

bool Error::was_emitted()
{
  return _count != 0;
}
