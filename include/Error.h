#pragma once

#include "AST.h"

// ---------------------------------------------
//  Error
// ---------------------------------------------

class Error {
public:
  struct ErrLoc {
    enum LocationType {
      LOC_AST,
      LOC_Token
    };

    LocationType type;

    union {
      AST::Base const* ast;
      Token const* token;
    };

    ErrLoc(AST::Base const* ast);
    ErrLoc(Token const& token);
  };

  enum ErrorLevel {
    EL_Error,
    EL_Warning,
    EL_Note
  };

  Error(ErrLoc loc, std::string const& msg)
      : loc(loc),
        msg(msg)
  {
  }

  Error& emit(ErrorLevel level = EL_Error);

  [[noreturn]] void exit(int code = 1);

  static bool was_emitted();

private:
  ErrLoc loc;
  std::string msg;
};
