#include <fstream>
#include <cassert>
#include <filesystem>

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

ScriptFileContext::ScriptFileContext(std::string const& path)
    : _is_open(false),
      _file_path(std::filesystem::canonical(path)),
      _ast(nullptr),
      _owner(nullptr)
{
}

ScriptFileContext::~ScriptFileContext()
{
  if (this->_ast)
    delete this->_ast;
}

//
// open the file
bool ScriptFileContext::open_file()
{
  if (this->_is_open)
    return false;

  std::ifstream ifs{this->_file_path};

  if (ifs.fail()) {
    return false;
  }

  for (std::string line; std::getline(ifs, line);) {
    this->_source_code += line + '\n';
  }

  return true;
}

//
// import a script file
bool ScriptFileContext::import(std::string const& path,
                               Token const& token,
                               AST::Scope* add_to)
{
  auto& ctx = this->_imported.emplace_back(path);

  if (!ctx.open_file()) {
    Error(token, "cannot open file '" + path + "'").emit();

    return false;
  }

  if (!ctx.lex())
    return false;

  if (!ctx.parse())
    return false;

  for (auto&& ast : ctx._ast->list) {
    add_to->append(ast);
    ast = nullptr;
  }

  ctx._owner = this;

  return true;
}

bool ScriptFileContext::lex()
{
  Lexer lexer{*this};

  this->_token_list = lexer.lex();

  return !Error::was_emitted();
}

bool ScriptFileContext::parse()
{
  Parser parser{*this, this->_token_list};

  this->_ast = parser.parse();

  assert(this->_ast->kind == AST_Scope);

  return !Error::was_emitted();
}

bool ScriptFileContext::check()
{
  Sema sema{this->_ast};

  sema.check(this->_ast);

  return !Error::was_emitted();
}

void ScriptFileContext::evaluate()
{
  Evaluator eval;

  auto result = eval.evaluate(this->_ast);

  delete result;
}

std::string const& ScriptFileContext::get_path() const
{
  return this->_file_path;
}

std::string const& ScriptFileContext::get_source_code() const
{
  return this->_source_code;
}

std::vector<ScriptFileContext> const&
ScriptFileContext::get_imported_list() const
{
  return this->_imported;
}

ScriptFileContext const* ScriptFileContext::is_imported(
    std::string const& path) const
{
  for (auto&& ctx : this->_imported) {
    if (ctx._file_path == path)
      return &ctx;

    if (auto p = ctx.is_imported(path); p)
      return p;
  }

  return nullptr;
}