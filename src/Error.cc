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
  typedef int64_t i64;

  // auto& app = Application::get_current_instance();

  // auto const& source = app.get_source_code();

  i64 errpos = 0;
  Token const* ptoken = nullptr;

  // ソースコード上の位置を取得
  switch (this->loc.type) {
    case ErrLoc::LOC_AST:
      ptoken = &this->loc.ast->token;
      errpos = this->loc.ast->token.src_loc.position;
      break;

    case ErrLoc::LOC_Token:
      ptoken = this->loc.token;
      errpos = this->loc.token->src_loc.position;
      break;
  }

  auto pcontext = ptoken->src_loc.context;

  std::string const& source = ptoken->src_loc.get_source();

  /// 行を切り取る
  i64 line_num = 1;
  i64 line_begin = 0;
  i64 line_end = source.length();

  // 開始位置
  for (i64 xx = 0; xx < errpos; xx++) {
    if (source[xx] == '\n') {
      line_num++;
      line_begin = xx + 1;
    }
  }

  // 終了位置
  for (i64 xx = errpos; xx < (signed)source.length(); xx++) {
    if (source[xx] == '\n') {
      line_end = xx;
      break;
    }
  }

  i64 err_ptr_char_pos = errpos - line_begin;

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