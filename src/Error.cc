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
      token(&token)
{
}

Error::ErrLoc::ErrLoc(AST::Base const* ast)
    : type(LOC_AST),
      ast(ast)
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
  switch (this->loc.type) {
    case ErrLoc::LOC_AST: {
      p_token = &this->loc.ast->token;
      p_end_token = &*this->loc.ast->end_token;
      errpos = this->loc.ast->token.src_loc.position;
      errpos_end =
          this->loc.ast->end_token->src_loc.get_end_pos();
      break;
    }

    case ErrLoc::LOC_Token:
      p_token = p_end_token = this->loc.token;
      errpos = this->loc.token->src_loc.position;
      errpos_end = this->loc.token->src_loc.get_end_pos();
      break;
  }

  assert(p_end_token);

  auto pcontext = p_token->src_loc.context;

  auto const& linerange_begin =
      pcontext->_srcdata._lines[p_token->src_loc.line_num - 1];

  auto const& linerange_end =
      pcontext->_srcdata
          ._lines[p_end_token->src_loc.line_num - 1];

  std::string const& source = pcontext->get_source_code();

  /// 行を切り取る
  size_t line_num = 1;
  size_t line_begin_pos = 0;
  size_t line_end_pos = source.length();

  // 開始位置
  for (size_t xx = 0; xx < errpos; xx++) {
    if (source[xx] == '\n') {
      line_num++;
      line_begin_pos = xx + 1;
    }
  }

  // 終了位置
  for (size_t xx = errpos; xx < source.length(); xx++) {
    if (source[xx] == '\n') {
      line_end_pos = xx;
      break;
    }
  }

  auto const err_line =
      p_token->src_loc.context->_srcdata.get_line(*p_token);

  size_t err_ptr_char_pos = errpos - line_begin_pos;

  std::cout << std::endl;

  // レベルによって最初の表示を変える
  switch (level) {
      // エラー
    case EL_Error:
      std::cout << COL_BOLD COL_RED "error: " << COL_WHITE
                << this->msg;
      break;

      // 警告
    case EL_Warning:
      std::cout << COL_BOLD COL_MAGENTA "warning: " << COL_WHITE
                << this->msg;
      break;

      // ヒント、ヘルプなど
    case EL_Note:
      std::cout << COL_BOLD COL_CYAN "note: " << COL_WHITE
                << this->msg;
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

void Error::exit(int code)
{
  std::exit(code);
}

bool Error::was_emitted()
{
  return __was_emitted;
}