#include <iostream>
#include <cassert>

#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Error.h"
#include "ScriptFileContext.h"
#include "Application.h"

static bool __was_emitted;

Error::ErrLoc::ErrLoc(Token const& token)
    : type(LOC_Token),
      ast(nullptr),
      token(&token)
{
}

Error::ErrLoc::ErrLoc(AST::Base const* ast)
    : type(LOC_AST),
      ast(ast),
      token(nullptr)
{
}

Error::Error(ErrLoc loc, std::string const& msg)
    : _loc(loc),
      _msg(msg)
{
}

static std::tuple<Token*, Token*> get_ast_token()
{
}

Error& Error::emit(ErrorLevel level)
{
  using LineRange = ScriptFileContext::LineRange;

  Token const* p_token = nullptr;
  Token const* p_end_token = nullptr;

  size_t errpos = 0;
  size_t errpos_end = 0;

  // ソースコード上の位置を取得
  switch (this->_loc.type) {
    case ErrLoc::LOC_AST: {
      p_token = &this->_loc.ast->token;
      p_end_token = &*this->_loc.ast->end_token;
      errpos = this->_loc.ast->token.src_loc.position;
      errpos_end =
          this->_loc.ast->end_token->src_loc.get_end_pos();
      break;
    }

    case ErrLoc::LOC_Token:
      p_token = p_end_token = this->_loc.token;
      errpos = this->_loc.token->src_loc.position;
      errpos_end = this->_loc.token->src_loc.get_end_pos();
      break;
  }

  assert(p_end_token);

  auto pcontext = p_token->src_loc.context;

  auto const& linerange_begin =
      pcontext->_srcdata._lines[p_token->src_loc.line_num - 1];

  auto const& linerange_end =
      pcontext->_srcdata
          ._lines[p_end_token->src_loc.line_num - 1];

  // SourceData
  auto const& src_data = pcontext->_srcdata;

  // LineRange
  auto const& line_range =
      src_data._lines[p_token->src_loc.line_num - 1];

  // source string
  auto const& source = pcontext->get_source_code();

  // line of error
  auto const err_line = src_data.get_line(*p_token);

  auto const line_num = line_range.index + 1;
  auto const line_begin_pos = line_range.begin;

  size_t err_ptr_char_pos = errpos - line_begin_pos;

  std::cout << std::endl;

  // レベルによって最初の表示を変える
  switch (level) {
      // エラー
    case EL_Error:
      std::cout << COL_BOLD COL_RED "error: " << COL_WHITE
                << this->_msg;
      break;

      // 警告
    case EL_Warning:
      std::cout << COL_BOLD COL_MAGENTA "warning: " << COL_WHITE
                << this->_msg;
      break;

      // ヒント、ヘルプなど
    case EL_Note:
      std::cout << COL_BOLD COL_CYAN "note: " << COL_WHITE
                << this->_msg;
      break;
  }

  // エラーが起きたファイルと行番号
  std::cerr << std::endl
            << COL_GREEN "    --> " << _RGB(0, 255, 255)
            << pcontext->get_path() << ":" << line_num
            << std::endl;

  // エラーが起きた行
  std::cerr << COL_DEFAULT COL_WHITE << "     |\n"
            << Utils::format("%4d |", line_num) << err_line;

  // 矢印
  std::cerr << COL_DEFAULT "     |"
            << std::string(err_ptr_char_pos, ' ') << '^'
            << std::endl
            << std::endl;

  if (level == EL_Error)
    __was_emitted = true;

  return *this;
}

Error& Error::emit2(ErrorLevel level)
{
}

void Error::exit(int code)
{
  std::exit(code);
}

bool Error::was_emitted()
{
  return __was_emitted;
}
