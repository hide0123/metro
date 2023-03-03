#pragma once

#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "../color.h"

#define ENABLE_CDSTRUCT 0

#define _RGB MAKE_COLOR
#define _BRGB MAKE_BK_COLOR

#define TAG_ALERT COL_YELLOW "#alert"
#define TAG_ALERTMSG COL_MAGENTA "#alertmsg "
#define TAG_ALERTCTOR _RGB(100, 200, 255) "#Contruct"
#define TAG_ALERTDTOR _RGB(200, 150, 60) "#Destruct"
#define TAG_TODOIMPL _RGB(50, 255, 255) "#not implemented here"
#define TAG_PANIC COL_RED "panic! "

#define _streamalert(tag, e...)                             \
  ({                                                        \
    std::stringstream ss;                                   \
    ss << e;                                                \
    _alert_impl(tag, ss.str().c_str(), __FILE__, __LINE__); \
  })

#if METRO_DEBUG

#define debug(...) __VA_ARGS__

#define alert _alert_impl(TAG_ALERT, nullptr, __FILE__, __LINE__)

#define alertmsg(e...) _streamalert(TAG_ALERTMSG, COL_WHITE << e)

#if ENABLE_CDSTRUCT
#define alert_ctor \
  _streamalert(TAG_ALERTCTOR, __func__ << "  " << this)

#define alert_dtor \
  _streamalert(TAG_ALERTDTOR, __func__ << " " << this)
#else
#define alert_ctor 0
#define alert_dtor 0
#endif

#else

#define debug(...) 0;
#define alert 0
#define alertmsg(...) 0
#define alert_ctor 0
#define alert_dtor 0

#endif

#define todo_impl \
  _alert_impl(TAG_TODOIMPL, nullptr, __FILE__, __LINE__), exit(1)

#define panic(e...)             \
  {                             \
    _streamalert(TAG_PANIC, e); \
    std::exit(222);             \
  }

inline char* _make_location_str(char* buf, char const* file,
                                size_t line)
{
  sprintf(buf, "%s:%zu:", strstr(file, "src"), line);
  return buf;
}

inline void _alert_impl(char const* tag, char const* msg,
                        char const* file, size_t line)
{
  char buf[0x100];
  char buf2[0x400]{' '};

  _make_location_str(buf, file, line);
  auto len = sprintf(
      buf2, COL_BOLD _BRGB(20, 20, 20) "        %s%-30s %s %s",
      _RGB(150, 255, 0), buf, tag, msg ? msg : "");

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
