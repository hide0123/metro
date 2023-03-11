#include <iostream>
#include <cassert>

#include "Utils.h"
#include "debug/alert.h"

#include "AST.h"
#include "Error.h"
#include "ScriptFileContext.h"
#include "Application.h"

size_t Error::_count;

ErrorLocation::ErrorLocation(Token const& token)
    : loc_kind(ERRLOC_Token),
      ast(nullptr),
      token(&token),
      is_single_line(false),
      _pos_begin(token.src_loc.position),
      _pos_end(token.src_loc.get_end_pos())
{
}

ErrorLocation::ErrorLocation(AST::Base const* ast)
    : loc_kind(ERRLOC_AST),
      ast(ast),
      token(nullptr),
      is_single_line(false),
      _pos_begin(ast->token.src_loc.position),
      _pos_end(ast->end_token->src_loc.get_end_pos())
{
  assert(&*ast->end_token);
}

Error::Error(ErrorLocation&& loc, std::string const& msg)
    : _kind(ERR_None),
      _msg(msg),
      _loc(std::move(loc))
{
}

Error::Error(ErrorKind kind, ErrorLocation&& loc,
             std::string const& msg)
    : _kind(kind),
      _msg(msg),
      _loc(std::move(loc))
{
}

Error& Error::single_line()
{
  this->_loc.is_single_line = true;
  return *this;
}

std::pair<Token const*, Token const*> Error::get_token_range()
    const
{
  Token const* begin = nullptr;
  Token const* end = nullptr;

  // ソースコード上の位置を取得
  switch (this->_loc.loc_kind) {
    case ERRLOC_AST: {
      begin = &this->_loc.ast->token;
      end = &*this->_loc.ast->end_token;
      break;
    }

    case ERRLOC_Token:
      begin = end = this->_loc.token;
      break;
  }

  assert(begin);
  assert(end);

  return {begin, end};
}

Error& Error::emit(ErrorLevel level)
{
  struct LineWrapper {
    ScriptFileContext::LineRange const& range;

    std::string str;

    LineWrapper(ScriptFileContext::LineRange const& R)
        : range(R)
    {
    }
  };

  auto [p_token, p_end_token] = this->get_token_range();

  auto pcontext = p_token->src_loc.context;

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
            << pcontext->get_path() << ":"
            << p_token->src_loc.line_num << std::endl;

  // auto const err_line =
  //     p_token->src_loc.context->_srcdata.get_line(*p_token);

  std::vector<LineWrapper> error_lines;

  auto const& source = pcontext->get_source_code();

  // size_t err_ptr_char_pos = errpos - line_begin_pos;

  // エラーが発生した行を切り取る
  {
    auto begin = p_token->src_loc.line_num - 1;
    auto const end = p_end_token->src_loc.line_num - 1;

    auto const& src_data = pcontext->_srcdata;

    do {
      auto& el =
          error_lines.emplace_back(src_data._lines[begin]);

      el.str = src_data.get_line(el.range);
    } while (++begin <= end);
  }

  // エラーが起きた行
  for (auto&& line : error_lines) {
    std::cerr << COL_DEFAULT COL_WHITE << "     |\n"
              << Utils::format("%4d |", line.range.index + 1)
              << line.str << COL_DEFAULT;
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
