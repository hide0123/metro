#include "Token.h"
#include "ScriptFileContext.h"

std::string& SourceLoc::get_source()
{
  return this->context->get_source_code();
}
