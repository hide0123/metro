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

  // Syntaxes
  ERR_InvalidSyntax,
  ERR_UnexpectedToken,
  ERR_EmptySwitch,
  ERR_EmptyStruct,

  // Semantics
  ERR_TypeMismatch,
  ERR_InvalidInitializer,
  ERR_UnexpectedType,
  ERR_ReturrnOutSideFunction,
  ERR_Undefined,
  ERR_MultipleDefined,

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

  ErrorLocation(AST::Base const* ast);
  ErrorLocation(Token const& token);

private:
  size_t const _line_num;

  friend class Error;
};

class ScriptFileContext;
class Error {
public:
  Error(Error&&) = delete;
  Error(Error const&) = delete;

  Error(ErrorKind kind, ErrorLocation&& loc,
        std::string const& msg);

  Error(ErrorLocation&& loc, std::string const& msg);

  Error& single_line();

  Error& emit(ErrorLevel level = EL_Error);

  [[noreturn]] void except();

  [[noreturn]] void exit(int code = 1);

  static bool was_emitted();

private:
  std::pair<Token const*, Token const*> get_token_range()
      const;
  void show_error_lines();

  ErrorKind _kind;
  ErrorLocation _loc;

  bool _is_single_line;
  std::string const _msg;

  ScriptFileContext const* _pContext;

  static size_t _count;
};
