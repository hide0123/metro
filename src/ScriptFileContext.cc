#include <fstream>
#include <cassert>
#include <filesystem>

#include "Utils.h"
#include "Debug.h"

#include "AST/AST.h"
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

SFContext::SourceData::SourceData(SourceData&& sd)
  : _path(std::move(sd._path)),
    _data(std::move(sd._data)),
    _lines(std::move(sd._lines))
{
}

SFContext::SourceData::~SourceData()
{
}

SFContext::LineView::LineView(size_t index, size_t begin, size_t end)
  : index(index),
    begin(begin),
    end(end)
{
}

SFContext::LineView const* SFContext::SourceData::find_line_range(
  size_t srcpos) const
{
  for (auto&& range : this->_lines) {
    if (range.begin <= srcpos && srcpos <= range.end)
      return &range;
  }

  return nullptr;
}

std::string_view SFContext::SourceData::get_line(
  SFContext::LineView const& line) const
{
  return {this->_data.data() + line.begin, line.end - line.begin + 1};
}

std::string_view SFContext::SourceData::get_line(Token const& token) const
{
  return this->get_line(this->_lines[token.src_loc.line_num - 1]);
}

SFContext::ScriptFileContext(std::string const& path)
  : _is_open(false),
    _in_cmdline(false),
    _srcdata(path),
    _ast(nullptr),
    _owner(nullptr),
    _importer_token(nullptr)
{
}

SFContext::ScriptFileContext(SFContext&& c)
  : _is_open(c._is_open),
    _in_cmdline(c._in_cmdline),
    _srcdata(std::move(c._srcdata)),
    _token_list(std::move(c._token_list)),
    _ast(c._ast),
    _owner(c._owner),
    _importer_token(c._importer_token),
    _imported(std::move(c._imported))
{
  c._is_open = false;
  c._ast = nullptr;
  c._owner = nullptr;
  c._importer_token = nullptr;
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

    line_pos = this->_srcdata._lines
                 .emplace_back(index, line_pos, line_pos + line.length() - 1)
                 .end +
               1;

    index++;
  }

  for (auto&& lview : this->_srcdata._lines) {
    lview.str_view = {this->_srcdata._data.data() + lview.begin,
                      lview.end - lview.begin};
  }

  return true;
}

//
// import a script file
bool SFContext::import(std::string const& path, Token const& token,
                       AST::Scope* add_to)
{
  auto& ctx = this->_imported.emplace_back(path);

  if (!ctx.open_file()) {
    Error(token, "cannot open file '" + path + "'").emit().exit();
  }

  ctx._owner = this;
  ctx._importer_token = &token;

  auto pContext = Application::get_instance()->get_context(ctx.get_path());

  if (pContext && pContext != &ctx) {
    for (auto p = this->_owner; p; p = p->_owner) {
      if (p->get_path() == ctx.get_path()) {
        Error(token, "cannot import recursively").emit();

        if (p->_importer_token) {
          Error(*p->_importer_token, "first imported here")
            .emit(EL_Note)
            .exit();
        }
        else {
          Error(token, "'" + path +
                         "' is already opened by argument in "
                         "command line")
            .emit(EL_Note)
            .exit();
        }
      }
    }

    if (pContext->get_path() == this->get_path())
      Error(token, "cannot import self").emit().exit();

    Error(token, "cannot import same file twice").emit().exit();
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

#if METRO_DEBUG
  if (auto& D = Debug::get_instance(); D.flags.ShowASTAfterParsed) {
    std::cout << "---- Parsed tree of " << this->get_path() << ": ---------\n"
              << AST::Base::to_string(this->_ast, D.flags.AddASTInfo,
                                      D.flags.AddTokenInfo)
              << std::endl
              << "----------------------------------------------------\n"
              << std::endl;
  }
#endif

  assert(this->_ast->kind == AST_Scope);

  return !Error::was_emitted();
}

bool SFContext::check()
{
  Sema sema{this->_ast};

  sema.do_check();

#if METRO_DEBUG
  if (auto& D = Debug::get_instance(); D.flags.ShowASTAfterChecked) {
    std::cout << "---- Checked tree of " << this->get_path() << ": ---------\n"
              << AST::Base::to_string(this->_ast, D.flags.AddASTInfo,
                                      D.flags.AddTokenInfo)
              << std::endl
              << "----------------------------------------------------\n"
              << std::endl;
  }
#endif

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
  if (!this->_in_cmdline && !this->open_file()) {
    std::cout << "metro: cannot open file '" << this->get_path() << "'"
              << std::endl;

    return;
  }

  try {
    alert;
    if (!this->lex())
      return;

    alert;
    if (!this->parse())
      return;

    if (!this->check())
      return;

    this->evaluate();
  }
  catch (Error& e) {
    e.emit().exit();
  }
}

std::string const& SFContext::get_path() const
{
  return this->_srcdata._path;
}

std::string& SFContext::get_source_code()
{
  return this->_srcdata._data;
}

std::vector<ScriptFileContext> const& SFContext::get_imported_list() const
{
  return this->_imported;
}

ScriptFileContext const* SFContext::is_imported(std::string const& path) const
{
  for (auto&& ctx : this->_imported) {
    if (ctx.get_path() == path)
      return &ctx;

    if (auto p = ctx.is_imported(path); p)
      return p;
  }

  return nullptr;
}

ScriptFileContext ScriptFileContext::from_cmdline(std::string&& source)
{
  ScriptFileContext ret{"<command-line>"};

  ret._is_open = true;
  ret._in_cmdline = true;

  auto& src = ret._srcdata;

  src._data = std::move(source);

  LineView lview{0, 0, src._data.length()};

  lview.str_view = src._data;

  src._lines = {lview};

  return ret;
}
