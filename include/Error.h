#pragma once

#include "AST.h"

// ---------------------------------------------
//  Error
// ---------------------------------------------

class Error {
public:
  struct ErrLoc {
    enum LocationType {
      LOC_Position,
      LOC_Token,
      LOC_AST
    };

    LocationType type;

    union {
      size_t pos;
      Token const* token;
      AST::Base const* ast;
    };

    ErrLoc(size_t pos);
    ErrLoc(Token const& token);
    ErrLoc(AST::Base const* ast);
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

  [[noreturn]]
  void exit(int code = 1);

  static bool was_emitted();

private:

  ErrLoc loc;
  std::string msg;
};

