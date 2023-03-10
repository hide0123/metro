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

using SFContext = ScriptFileContext;

SFContext::SourceData::SourceData(std::string const& path)
    : _path(path)
{
}

SFContext::LineRange const*
SFContext::SourceData::find_line_range(size_t srcpos) const
{
  for (auto&& range : this->_lines) {
    if (range.begin <= srcpos && srcpos <= range.end)
      return &range;
  }

  return nullptr;
}

std::string_view SFContext::SourceData::get_line(
    SFContext::LineRange const& line) const
{
  return {this->_data.data() + line.begin,
          line.end - line.begin};
}

std::string_view SFContext::SourceData::get_line(
    Token const& token) const
{
  return this->_data.substr(token.src_loc.position,
                            token.src_loc.length);
}

SFContext::ScriptFileContext(std::string const& path)
    : _is_open(false),
      _srcdata(std::filesystem::canonical(path)),
      _ast(nullptr),
      _owner(nullptr),
      _importer_token(nullptr)
{
  debug(std::cout << this->_srcdata._path << std::endl);
}

SFContext::~ScriptFileContext()
{
  if (this->_ast)
    delete this->_ast;
}

bool SFContext::is_opened() const
{
  return this->_is_open;
}

//
// open the file
bool SFContext::open_file()
{
  if (this->_is_open)
    return false;

  std::ifstream ifs{this->_srcdata._path};

  if (ifs.fail()) {
    return false;
  }

  size_t index = 0;
  size_t line_pos = 0;

  for (std::string line; std::getline(ifs, line);) {
    line += '\n';
    this->_srcdata._data += line;

    line_pos +=
        this->_srcdata._lines
            .emplace_back(LineRange{index++, line_pos,
                                    line_pos + line.length()})
            .end;
  }

  return true;
}

//
// import a script file
bool SFContext::import(std::string const& path,
                       Token const& token, AST::Scope* add_to)
{
  auto& ctx = this->_imported.emplace_back(path);

  ctx._owner = this;
  ctx._importer_token = &token;

  auto found =
      Application::get_instance()->get_context(ctx.get_path());

  if (found && found != &ctx) {
    for (auto p = this->_owner; p; p = p->_owner) {
      if (p->get_path() == ctx.get_path()) {
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

bool SFContext::lex()
{
  Lexer lexer{*this};

  this->_token_list = lexer.lex();

  return !Error::was_emitted();
}

bool SFContext::parse()
{
  Parser parser{*this, this->_token_list};

  this->_ast = parser.parse();

  assert(this->_ast->kind == AST_Scope);

  return !Error::was_emitted();
}

bool SFContext::check()
{
  Sema sema{this->_ast};

  sema.check(this->_ast);

  return !Error::was_emitted();
}

Object* SFContext::evaluate()
{
  Evaluator eval;

  auto result = eval.evaluate(this->_ast);

  return result;
}

void SFContext::execute_full()
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

std::string const& SFContext::get_path() const
{
  return this->_srcdata._path;
}

std::string const& SFContext::get_source_code() const
{
  return this->_srcdata._data;
}

std::vector<ScriptFileContext> const&
SFContext::get_imported_list() const
{
  return this->_imported;
}

ScriptFileContext const* SFContext::is_imported(
    std::string const& path) const
{
  for (auto&& ctx : this->_imported) {
    if (ctx.get_path() == path)
      return &ctx;

    if (auto p = ctx.is_imported(path); p)
      return p;
  }

  return nullptr;
}