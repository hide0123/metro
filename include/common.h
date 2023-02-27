#pragma once

#include <string>
#include <vector>

// #define METRO_DEBUG 1

#define COL_DEFAULT   "\033[0m"
#define COL_BOLD      "\033[1m"

#define COL_BLACK     "\033[30m"
#define COL_RED       "\033[31m"
#define COL_GREEN     "\033[32m"
#define COL_YELLOW    "\033[33m"
#define COL_BLUE      "\033[34m"
#define COL_MAGENTA   "\033[35m"
#define COL_CYAN      "\033[36;5m"
#define COL_WHITE     "\033[37m"

#define COL_BK_BLACK     "\033[40m"
#define COL_BK_RED       "\033[41m"
#define COL_BK_GREEN     "\033[42m"
#define COL_BK_YELLOW    "\033[43m"
#define COL_BK_BLUE      "\033[44m"
#define COL_BK_MAGENTA   "\033[45m"
#define COL_BK_CYAN      "\033[46;5m"
#define COL_BK_WHITE     "\033[47m"


#define __THISFILE__ \
  (__FILE__ + std::size("/home/letz/sources/cpp/letzc"))

#if METRO_DEBUG
  #include <iostream>
  #include <cstring>
  #include <cassert>
  #include <sstream>

  #define debug(...) __VA_ARGS__

  #define alert \
    fprintf(stderr,COL_YELLOW "\t#alert at " \
      COL_GREEN "%s:%d" COL_DEFAULT "\n",__THISFILE__,__LINE__)

  // #define print_variable(fmt, v) \
  //   fprintf(stderr,"\t#viewvar: " #v " = " fmt "\n", v)

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

  template <size_t len, size_t _n>
  inline char const* rightspace(char const (&str) [_n]) {
    static char buf[0x100];

    // static_assert(_n < len);
    if constexpr (_n >= len) {
      return str;
    }

    memcpy(buf, str, _n);
    memset(buf + _n, ' ', len - _n);
    buf[len] = 0;

    return buf;
  }

  template <size_t len>
  inline std::string rightspace(std::string const& str) {
    if( str.length() < len ) {
      return str + std::string(len - str.length(), ' ');
    }

    return str;
  }

  inline std::string __makealertmsgstr(
    std::string const& loc, auto&&... args) {
    std::stringstream ss;

    ss <<
      COL_BOLD "\t#alertmsg " COL_BLACK << (... << args);

    return rightspace<40>(ss.str())
        + " " COL_GREEN + loc + COL_DEFAULT;
  }
#else
  #define debug(...) 0;

  #define alert 0
  #define print_variable(...) 0
  #define panic(...) 0
  #define alert_ctor 0
  #define alert_dtor 0
#endif

#define todo_impl panic("implement here %s", __func__)

#define TODO todo_impl

#define alertmsg(a...) \
  debug(std::cout<<Color::Gray.to_bgstr()<< \
  Color::Magenta.to_str()<< __makealertmsgstr(Utils::format( \
  "%s:%d" COL_DEFAULT "\n",__THISFILE__,(int)__LINE__),[&]()\
  {std::stringstream _;_<<a;return _.str();}()));

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

struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;

  Color(uint8_t r, uint8_t g, uint8_t b)
    : r(r),
      g(g),
      b(b)
  {
  }

  Color(uint32_t rgb)
    : r(rgb | 0xFF0000),
      g(rgb | 0x00FF00),
      b(rgb | 0x0000FF)
  {
  }

  std::string to_str() const {
    char aa[50];
    sprintf(aa, "\033[38;2;%d;%d;%dm\0", r, g, b);
    return aa;
  }

  std::string to_bgstr() const {
    auto s = this->to_str();

    s[2] = '4';

    return s;
  }

  static Color const White;
  static Color const Gray;
  static Color const Yellow;
  static Color const Red;
  static Color const Blue;
  static Color const Green;
  static Color const Magenta;
  static Color const Cyan;


};
