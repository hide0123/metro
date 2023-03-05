#include <fstream>
#include <cassert>

#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Object.h"

#include "Lexer.h"
#include "Parser.h"
#include "Sema.h"
#include "Evaluator.h"

#include "Application.h"
#include "ScriptFileContext.h"

#include "Error.h"

bool ScriptFileContext::open_file()
{
  if (this->is_open)
    return false;

  std::ifstream ifs{this->file_path};

  if (ifs.fail()) {
    return false;
  }

  for (std::string line; std::getline(ifs, line);) {
    this->source_code += line + '\n';
  }

  return true;
}

bool ScriptFileContext::import(std::string const& path,
                               Token const& token,
                               AST::Scope* add_to)
{
  auto& ctx = this->imported.emplace_back(path);

  if (!ctx.open_file()) {
    Error(token, "cannot open file '" + path + "'").emit();

    return false;
  }

  if (!ctx.lex())
    return false;

  if (!ctx.parse())
    return false;

  for (auto&& ast : ctx.ast->list) {
    add_to->append(ast);
    ast = nullptr;
  }

  return true;
}

bool ScriptFileContext::lex()
{
  Lexer lexer{*this};

  this->token_list = lexer.lex();

  for (auto&& token : this->token_list) {
    token.src_loc.context = this;
  }

  return !Error::was_emitted();
}

bool ScriptFileContext::parse()
{
  Parser parser{*this, this->token_list};

  this->ast = parser.parse();

  assert(this->ast->kind == AST_Scope);

  return !Error::was_emitted();
}

bool ScriptFileContext::check()
{
  Sema sema{this->ast};

  sema.check(this->ast);

  return !Error::was_emitted();
}

void ScriptFileContext::evaluate()
{
  Evaluator eval;

  auto result = eval.evaluate(this->ast);

  delete result;
}

std::string const& ScriptFileContext::get_path() const
{
  return this->file_path;
}

std::string const& ScriptFileContext::get_source_code() const
{
  return this->source_code;
}

ScriptFileContext::ScriptFileContext(std::string const& path)
    : is_open(false),
      file_path(path),
      ast(nullptr)
{
}

ScriptFileContext::~ScriptFileContext()
{
  if (this->ast)
    delete ast;
}
