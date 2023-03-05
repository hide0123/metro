#include "Token.h"
#include "ScriptFileContext.h"

std::string const& SourceLoc::get_source() const
{
  return this->context->get_source_code();
}
