/* debugging util */
#pragma once

#include "Basics.H"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <set>
#include <string>

#define DBGt_INFO  0 // normal log
#define DBGt_FAIL  1 // fatal error
#define DBGt_ERR   2 // error
#define DBGt_WARN  3 // warning
#define DBGt_TODO  4 // todo

namespace Dbg
{

extern std::set<std::string> _ignored;

static void XFmtV(U64 dbgt, const char *scope, const char *msg, va_list args)
{
  if (_ignored.contains(scope) && dbgt != DBGt_FAIL) {
    return;
  }

  CStr pfx = "";
  switch (dbgt) {
    case DBGt_INFO: pfx = "I"; break;
    case DBGt_FAIL: pfx = "F"; break;
    case DBGt_ERR:  pfx = "E"; break;
    case DBGt_WARN: pfx = "W"; break;
    case DBGt_TODO: pfx = "T"; break;
    default:        pfx = "?"; break;
  }

  fprintf(stderr, "%s %8s ", pfx, scope);
  vfprintf(stderr, msg, args);
  fprintf(stderr, "\n");
}

static void Disable(std::set<std::string> scopes)
{
  _ignored = scopes;
}

[[noreturn]]
[[gnu::format(printf, 2, 3)]]
static void Fail(const char *scope, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  XFmtV(DBGt_FAIL, scope, msg, args);
  va_end(args);
  fflush(stderr);
  exit(1);
}

[[gnu::format(printf, 2, 3)]]
static void Err(const char *scope, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  XFmtV(DBGt_ERR, scope, msg, args);
  va_end(args);
}

[[gnu::format(printf, 2, 3)]]
static void Warn(const char *scope, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  XFmtV(DBGt_WARN, scope, msg, args);
  va_end(args);
}

[[gnu::format(printf, 2, 3)]]
static void Info(const char *scope, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  XFmtV(DBGt_INFO, scope, msg, args);
  va_end(args);
}

[[gnu::format(printf, 2, 3)]]
static void Todo(const char *scope, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  XFmtV(DBGt_TODO, scope, msg, args);
  va_end(args);
}

}
