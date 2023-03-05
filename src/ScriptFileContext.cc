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
      _owner(nullptr),
      _importer_token(nullptr)
{
  debug(std::cout << this->_file_path << std::endl);
}

ScriptFileContext::~ScriptFileContext()
{
  if (this->_ast)
    delete this->_ast;
}

bool ScriptFileContext::is_opened() const
{
  return this->_is_open;
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

  ctx._owner = this;
  ctx._importer_token = &token;

  if (auto find = Application::get_instance()->get_context(
          ctx._file_path);
      find && find != &ctx) {
    for (auto p = this->_owner; p; p = p->_owner) {
      if (p->_file_path == ctx._file_path) {
        Error(token, "cannot import recursively").emit();

        if (p->_importer_token) {
          Error(*p->_importer_token, "first imported here")
              .emit(Error::EL_Note)
              .exit();
        }
        else {
          Error(token, "'" + path +
                           "' is already opened by argument in "
                           "command line")
              .emit(Error::EL_Note)
              .exit();
        }
      }
    }

    Error(token, "cannot import self").emit().exit();
  }

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

Object* ScriptFileContext::evaluate()
{
  Evaluator eval;

  auto result = eval.evaluate(this->_ast);

  return result;
}

void ScriptFileContext::execute_full()
{
  if (!this->open_file()) {
    std::cout << "metro: cannot open file '" << this->get_path()
              << "'" << std::endl;

    return;
  }

  if (!this->lex())
    return;

  if (!this->parse())
    return;

  if (!this->check())
    return;

  auto result = this->evaluate();

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