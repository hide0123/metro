#pragma once

#include <string>
#include <vector>

#define metro_debug 1

#define COL_BLACK     "\e[30m"
#define COL_RED       "\e[31m"
#define COL_GREEN     "\033[32m"
#define COL_YELLOW    "\033[33m"
#define COL_BLUE      "\033[34m"
#define COL_MAGENTA   "\033[35m"
#define COL_CYAN      "\033[36;5m"
#define COL_WHITE     "\033[37m"
#define COL_DEFAULT   "\033[0;0;0m"

#define __THISFILE__ \
  (__FILE__ + std::size("/home/letz/sources/cpp/letzc"))

#if metro_debug
  #include <iostream>
  #include <cassert>

  #define debug(...) __VA_ARGS__

  #define alert \
    fprintf(stderr,COL_YELLOW "\t#alert at " \
      COL_GREEN "%s:%d" COL_DEFAULT "\n",__THISFILE__,__LINE__)

  #define print_variable(fmt, v) \
    fprintf(stderr,"\t#viewvar: " #v " = " fmt "\n", v)

  #define panic(fmt, ...) \
    fprintf(stderr, "\t#panic at %s:%d: " fmt "\n", \
      __FILE__, __LINE__, __VA_ARGS__), \
    exit(1)

  #define alert_ctor \
    debug(fprintf(stderr,"\t#alert_ctor at %s:%d: %s %p\n",__FILE__,__LINE__,\
    __func__,this))

  #define alert_dtor \
    debug(fprintf(stderr,"\t#alert_dtor at %s:%d: %s %p\n",__FILE__,__LINE__,\
    __func__,this))
#else
  #define debug(...)

  #define alert 0
  #define print_variable(...) 0
  #define panic(...) 0
  #define alert_ctor 0
  #define alert_dtor 0
#endif

#define todo_impl panic("implement here %s", __func__)

#define TODO todo_impl

namespace Utils {
  
template <class ... Args>
std::string format(char const* fmt, Args&&... args) {
  static char buf[0x1000];
  sprintf(buf, fmt, args...);
  return buf;
}

namespace String {

std::wstring to_wstr(std::string const& str);
std::string to_str(std::wstring const& str);

} // namespace Utils::String

} // namespace Utils
