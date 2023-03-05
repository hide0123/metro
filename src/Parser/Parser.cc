#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Parser.h"
#include "Error.h"

#include "ScriptFileContext.h"

Parser::Parser(ScriptFileContext& context,
               std::list<Token>& token_list)
    : _context(context),
      _token_list(token_list)
{
  this->cur = this->_token_list.begin();
  this->ate = this->_token_list.end();
}

Parser::~Parser()
{
}

AST::Scope* Parser::parse()
{
  auto root_scope = new AST::Scope(*this->cur);

  while (this->check()) {
    if (this->eat("import")) {
      auto const& token = *this->ate;
      std::string path;

      do {
        path += this->expect_identifier()->str;
      } while (this->eat("/"));

      path += ".metro";

      if (!this->_context.import(path, token, root_scope)) {
        Error(token, "failed to import file '" + path + "'")
            .emit()
            .exit();
      }

      continue;
    }

    root_scope->append(this->top());
  }

  return root_scope;
}