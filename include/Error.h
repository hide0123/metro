// ---------------------------------------------
//  Error
// ---------------------------------------------
#pragma once

#include <memory>
#include "AST.h"

enum ErrorKind {
  ERR_None,

  // Lexer
  ERR_InvalidToken,
  ERR_UnclosedStrignLiteral,

  // Parser
  ERR_InvalidSyntax,
  ERR_UnexpectedToken,

  // Sema
  ERR_TypeMismatch,
  ERR_UnexpectedType,
  ERR_ReturrnOutSideFunction,

  // run-time
  ERR_IndexOutOfRange,
};

enum ErrorLocationKind {
  ERRLOC_AST,
  ERRLOC_Token,
};

enum ErrorLevel {
  EL_Error,
  EL_Warning,
  EL_Note
};

class Error;
struct ErrorLocation {
  ErrorLocationKind loc_kind;

  AST::Base const* ast;
  Token const* token;

  // show all of error occurred codes
  bool is_single_line;

  ErrorLocation(AST::Base const* ast);
  ErrorLocation(Token const& token);

private:
  size_t const _pos_begin;
  size_t const _pos_end;

  friend class Error;
};

class Error {
public:
  Error(Error&&) = delete;
  Error(Error const&) = delete;

  Error(ErrorLocation&& loc, std::string const& msg);
  Error(ErrorKind kind, ErrorLocation&& loc,
        std::string const& msg);

  Error& single_line();

  Error& emit(ErrorLevel level = EL_Error);

  [[noreturn]] void except();

  [[noreturn]] void exit(int code = 1);

  static bool was_emitted();

private:
  std::pair<Token const*, Token const*> get_token_range() const;

  ErrorKind _kind;
  std::string const _msg;
  ErrorLocation _loc;

  static size_t _count;
};
