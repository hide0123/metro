#include <codecvt>
#include <locale>

#include "Utils.h"
#include "debug/alert.h"

namespace Utils::String {

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> conv;

std::wstring to_wstr(std::string const& str)
{
  return conv.from_bytes(str);
}

std::string to_str(std::wstring const& str)
{
  return conv.to_bytes(str);
}

}  // namespace Utils::String
