#pragma once

#include <string>
#include <cstring>

#define COL_DEFAULT "\033[0m"
#define COL_BOLD "\033[1m"

#define COL_BLACK "\033[30m"
#define COL_RED "\033[31m"
#define COL_GREEN "\033[32m"
#define COL_YELLOW "\033[33m"
#define COL_BLUE "\033[34m"
#define COL_MAGENTA "\033[35m"
#define COL_CYAN "\033[36m"
#define COL_WHITE "\033[37m"

#define COL_BK_BLACK "\033[40m"
#define COL_BK_RED "\033[41m"
#define COL_BK_GREEN "\033[42m"
#define COL_BK_YELLOW "\033[43m"
#define COL_BK_BLUE "\033[44m"
#define COL_BK_MAGENTA "\033[45m"
#define COL_BK_CYAN "\033[46m"
#define COL_BK_WHITE "\033[47m"

#define MAKE_COLOR(r, g, b) "\033[38;2;" #r ";" #g ";" #b "m"
#define MAKE_BK_COLOR(r, g, b) "\033[48;2;" #r ";" #g ";" #b "m"

inline std::string make_color_string(uint8_t r, uint8_t g, uint8_t b,
                                     bool is_bk = false)
{
  char buf[100];
  sprintf(buf, "\033[%s;2;%d;%d;%dm", is_bk ? "48" : "38", r, g, b);
  return buf;
}