#pragma once

#include <string>
#include <vector>

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
    sprintf(aa, "\033[38;2;%d;%d;%dm", r, g, b);
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

template <class T>
using pure_type =
  std::remove_reference<
    std::remove_cv_t<T>>;
