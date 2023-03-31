#pragma once

#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "color.h"

#define _RGB MAKE_COLOR
#define _BRGB MAKE_BK_COLOR

#if METRO_DEBUG && !METRO_NO_ALERT

#define TAG_ALERT COL_YELLOW "#alert"
#define TAG_ALERTMSG COL_MAGENTA "#alertmsg "
#define TAG_ALERTFMT COL_CYAN "#alertfmt "
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

#define debug(...)                                \
  ({                                              \
    if (Debug::get_instance().flags.DebugScope) { \
      __VA_ARGS__;                                \
    }                                             \
  })

#define alert _alert_impl(TAG_ALERT, nullptr, __FILE__, __LINE__)

#define alertmsg(e...) _streamalert(TAG_ALERTMSG, COL_WHITE << e)
#define alertfmt(e...)                             \
  ({                                               \
    static char _buf[0x200];                       \
    sprintf(_buf, e);                              \
    _streamalert(TAG_ALERTFMT, COL_WHITE << _buf); \
  })

#define alert_ctor                                           \
  ({                                                         \
    if (Debug::get_instance().flags.AlertConstructor)        \
      _streamalert(TAG_ALERTCTOR, __func__ << "  " << this); \
  })

#define alert_dtor                                           \
  ({                                                         \
    if (Debug::get_instance().flags.AlertDestructor)         \
      _streamalert(TAG_ALERTCTOR, __func__ << "  " << this); \
  })

class Application;
class Debug {
public:
  struct Flags {
    bool Alert;
    bool AlertConstructor;
    bool AlertDestructor;

    bool DebugScope;

    bool ShowParsedTree;
    bool ShowCheckedTree;
  };

  Flags flags{};

  static Debug const& get_instance();

private:
  friend class Application;

  Debug();
};

char* _make_location_str(char* buf, char const* file, size_t line);
void _alert_impl(char const* tag, char const* msg, char const* file,
                 size_t line);

#else

#define debug(...) (void)0;
#define alert (void)0
#define alertmsg(...) (void)0
#define alertfmt(...) (void)0
#define alert_ctor (void)0
#define alert_dtor (void)0

#endif

#define todo_impl                                                              \
  {                                                                            \
    printf(COL_RED "\t#todo_impl %s:%d" COL_DEFAULT "\n", __FILE__, __LINE__); \
    std::exit(999);                                                            \
  }

#define panic(e...)                                                         \
  {                                                                         \
    printf(COL_RED "\t#panic! %s:%d" COL_DEFAULT "\n", __FILE__, __LINE__); \
    std::exit(222);                                                         \
  }
