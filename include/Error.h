// ---------------------------------------------
//  Error
// ---------------------------------------------
#pragma once

#include <utility>
#include <tuple>
#include "AST.h"

enum ErrorKind {
};

struct ErrorLocation {
  enum LocationType {
    LOC_AST,
    LOC_Token
  };

  LocationType type;

  AST::Base const* ast;
  Token const* token;

  ErrorLocation(AST::Base const* ast);
  ErrorLocation(Token const& token);

private:
  size_t _pos_begin;
  size_t _pos_end;
};

class Error {
public:
  enum ErrorLevel {
    EL_Error,
    EL_Warning,
    EL_Note
  };

  Error(ErrLoc loc, std::string const& msg);

  Error& emit(ErrorLevel level = EL_Error);

  Error& emit2(ErrorLevel level = EL_Error);

  [[noreturn]] void exit(int code = 1);

  static bool was_emitted();

private:
  static std::pair<size_t, size_t> get_error_range_on_source();

  ErrLoc const _loc;
  std::string const _msg;
};
