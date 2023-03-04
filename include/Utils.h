#pragma once

#include <string>
#include <vector>

namespace Utils {

template <class... Args>
std::string format(char const* fmt, Args&&... args)
{
  static char buf[0x1000];
  sprintf(buf, fmt, args...);
  return buf;
}

namespace String {

std::wstring to_wstr(std::string const& str);
std::string to_str(std::wstring const& str);

}  // namespace String

}  // namespace Utils
