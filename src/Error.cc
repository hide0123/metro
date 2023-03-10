#include <iostream>

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
  Token const* ptoken = nullptr;

  size_t errpos = 0;
  size_t errpos_end = 0;
  size_t line_count = 1;

  // ソースコード上の位置を取得
  switch (this->loc.type) {
    case ErrLoc::LOC_AST: {
      ptoken = &this->loc.ast->token;
      errpos = this->loc.ast->token.src_loc.position;
      errpos_end =
          this->loc.ast->end_token->src_loc.get_end_pos();
      break;
    }

    case ErrLoc::LOC_Token:
      ptoken = this->loc.token;
      errpos = this->loc.token->src_loc.position;
      errpos_end = this->loc.token->src_loc.get_end_pos();
      break;
  }

  auto pcontext = ptoken->src_loc.context;

  std::string const& source = ptoken->src_loc.get_source();

  /// 行を切り取る
  size_t line_num = 1;
  size_t line_begin = 0;
  size_t line_end = source.length();

  // 開始位置
  for (size_t xx = 0; xx < errpos; xx++) {
    if (source[xx] == '\n') {
      line_num++;
      line_begin = xx + 1;
    }
  }

  // 終了位置
  for (size_t xx = errpos; xx < source.length(); xx++) {
    if (source[xx] == '\n') {
      line_end = xx;
      break;
    }
  }

  size_t err_ptr_char_pos = errpos - line_begin;

  auto const err_line =
      source.substr(line_begin, line_end - line_begin);

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
  std::cout << std::endl
            << COL_GREEN "    --> " << _RGB(0, 255, 255)
            << pcontext->get_path() << ":" << line_num
            << std::endl

            // エラーが起きた行
            << COL_DEFAULT COL_WHITE << "     |\n"
            << Utils::format("%4d |", line_num) << err_line
            << std::endl
            << COL_DEFAULT "     |"

            // 矢印
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