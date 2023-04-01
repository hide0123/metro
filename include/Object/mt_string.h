#pragma once

#include <string>
#include <codecvt>
#include <locale>

using metro_char_t = char16_t;

class metro_string_t : public std::basic_string<metro_char_t> {
  static inline std::wstring_convert<std::codecvt_utf8_utf16<metro_char_t>,
                                     metro_char_t>
    conv;

public:
  using std::basic_string<metro_char_t>::basic_string;

  std::string to_string() const
  {
    return conv.to_bytes(*this);
  }
};
