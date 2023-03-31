#if METRO_DEBUG

#include "Debug.h"

static Debug* _instance;

Debug::Debug()
{
  _instance = this;
}

Debug const& Debug::get_instance()
{
  return *_instance;
}

char* _make_location_str(char* buf, char const* file, size_t line)
{
  sprintf(buf, "%s:%zu:", strstr(file, "src"), line);
  return buf;
}

void _alert_impl(char const* tag, char const* msg, char const* file,
                 size_t line)
{
  if (!Debug::get_instance().flags.Alert)
    return;

  char buf[0x100];
  char buf2[0x400]{' '};
  char colstr[50];

  uint8_t col[3];

  for (auto&& c : col) {
    constexpr auto begin = 100;
    constexpr auto unit = 10;
    constexpr auto range = (255 - begin) / unit;

    c = begin + (rand() % range) * unit;
  }

  _make_location_str(buf, file, line);
  sprintf(colstr, "\033[38;2;%d;%d;%dm", col[0], col[1], col[2]);

  auto len = sprintf(buf2, COL_BOLD _BRGB(10, 10, 10) "        %s%-30s %s %s",
                     colstr, buf, tag, msg ? msg : "");

  size_t endpos = 200;

  for (int i = 0; i < len;) {
    if (buf2[i] == '\033') {
      do {
        endpos++;
        i++;
      } while (buf2[i] != 'm');
    }
    else {
      i++;
    }
  }

  memset(buf2 + len, ' ', 0x400 - len);
  buf2[endpos] = 0;

  printf("%s" COL_DEFAULT "\n", buf2);
}

#endif