#include <iostream>
#include <cassert>

#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Error.h"
#include "ScriptFileContext.h"
#include "Application.h"

void Error::trim_error_lines()
{
  auto [p_token, p_end_token] = this->get_token_range();

  auto begin = p_token->src_loc.line_num - 1;
  auto const end = p_end_token->src_loc.line_num - 1;

  auto const& src_data = this->_pContext->_srcdata;

  do {
    auto& lview = src_data._lines[begin];

    this->_error_lines.emplace_back(
        Utils::format("%4d |", lview.index + 1) +
        std::string(lview.str_view));
  } while (++begin <= end);
}

Error& Error::emit(ErrorLevel level)
{
  // auto pcontext = p_token->src_loc.context;

  // ScriptFileContext const* pContext = nullptr;
  size_t line_num = 0;

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
            << this->_pContext->get_path() << ":" << line_num
            << std::endl;

  auto const& source = this->_pContext->get_source_code();

  // size_t err_ptr_char_pos = errpos - line_begin_pos;

  // エラーが発生した行を切り取る
  // {
  //   auto [p_token, p_end_token] = this->get_token_range();

  //   auto begin = p_token->src_loc.line_num - 1;
  //   auto const end = p_end_token->src_loc.line_num - 1;

  //   pContext = p_token->src_loc.context;

  //   auto const& src_data = pContext->_srcdata;

  //   do {
  //     error_lines.emplace_back(src_data._lines[begin]);
  //   } while (++begin <= end);
  // }

  // エラーが起きた行
  this->trim_error_lines();

  std::cerr << COL_DEFAULT COL_WHITE << "     |\n";

  for (auto&& line : this->_error_lines) {
    std::cerr << Utils::format("%4d |", line_num++) << line
              << std::endl
              << COL_DEFAULT;
  }

  // 矢印
  std::cerr << COL_DEFAULT "     |" << std::string(1, ' ') << '^'
            << std::endl
            << std::endl;

  if (level == EL_Error) {
    _count++;
  }

  return *this;
}

void Error::exit(int code)
{
  std::exit(code);
}

bool Error::was_emitted()
{
  return _count != 0;
}
