#pragma once

#include <string>
#include <vector>

#define for_indexed(_Index, _Val, _Content)       \
  for (size_t _Index = 0; auto&& _Val : _Content) \
    if (_Index < _Content.size())                 \
      if (struct {                                \
            struct S {                            \
              size_t& _ref;                       \
              S(size_t& _)                        \
                : _ref(_)                         \
              {                                   \
              }                                   \
              ~S()                                \
              {                                   \
                this->_ref++;                     \
              }                                   \
            };                                    \
            S t;                                  \
          } u{_Index};                            \
          1)

namespace Utils {

template <class... Args>
std::string format(char const* fmt, Args&&... args)
{
  static char buf[0x1000];
  sprintf(buf, fmt, args...);
  return buf;
}

namespace String {

std::string join(std::string const& s, auto&& x, auto f)
{
  if (x.empty())
    return "";

  auto it = x.begin();
  auto ret = f(*it++);

  for (; it != x.end(); it++)
    ret += s + f(*it);

  return ret;
}

template <class T>
std::string join(std::string const& s, T&& x)
{
  return join(s, std::forward<T>(x), [](auto& w) {
    return std::to_string(w);
  });
}

std::wstring to_wstr(std::string const& str);
std::string to_str(std::wstring const& str);

}  // namespace String

}  // namespace Utils
