// ---------------------------------------------
//  Error
// ---------------------------------------------
#pragma once

#include <memory>
#include "AST/ASTfwd.h"
#include "color.h"

enum ErrorKind {
  ERR_None,

  // Lexer
  ERR_InvalidToken,
  ERR_UnclosedStrignLiteral,

  // Syntaxes
  ERR_InvalidSyntax,
  ERR_UnexpectedToken,

  // Semantics
  ERR_TypeMismatch,
  ERR_InvalidInitializer,
  ERR_UnexpectedType,
  ERR_ReturrnOutSideFunction,
  ERR_Undefined,
  ERR_MultipleDefined,
  ERR_EmptySwitch,
  ERR_EmptyEnum,
  ERR_EmptyStruct,

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
class ScriptFileContext;
struct ErrorLocation {
  ErrorLocationKind loc_kind;

  AST::Base const* ast;
  Token const* token;

  ErrorLocation(AST::Base const* ast);
  ErrorLocation(Token const& token);

private:
  size_t const _line_num;
  ScriptFileContext const* _context;

  friend class Error;
};

class Error {
public:
  Error(Error&&) = delete;
  Error(Error const&) = delete;

  Error(ErrorKind kind, ErrorLocation&& loc, std::string const& msg);

  Error(ErrorLocation&& loc, std::string const& msg);

  Error& single_line();

  Error& emit(ErrorLevel level = EL_Error);

  [[noreturn]] void except();

  [[noreturn]] void exit(int code = 1);

  static bool was_emitted();

  template <class... Args>
  static void fatal_error(Args&&... args)
  {
    std::cerr << COL_RED << "fatal error: " << COL_WHITE;

    (std::cerr << ... << std::forward<Args>(args)) << COL_DEFAULT << std::endl;

    std::exit(1);
  }

private:
  std::pair<Token const*, Token const*> get_token_range() const;
  void show_error_lines();

  ErrorKind _kind;
  ErrorLocation _loc;

  bool _is_single_line;
  std::string const _msg;

  bool _custom;
  Token const* _tbegin;
  Token const* _tend;

  ScriptFileContext const* _pContext;

  static size_t _count;
};
