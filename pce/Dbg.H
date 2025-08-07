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

namespace Dbg
{

std::set<std::string> _ignored;

void XFmtV(U64 dbgt, const char *scope, const char *msg, va_list args)
{
  if (_ignored.contains(scope)) {
    return;
  }

  CStr pfx = "";
  switch (dbgt) {
    case DBGt_INFO: pfx = "\x1b[34mI"; break;
    case DBGt_FAIL: pfx = "\x1b[31mF"; break;
    case DBGt_ERR:  pfx = "\x1b[31mE"; break;
    case DBGt_WARN: pfx = "\x1b[33mW"; break;
    default:        pfx = "\x1b[33m?"; break; 
  }

  fprintf(stderr, "%s \x1b[0m%8s \x1b[37m", pfx, scope);
  vfprintf(stderr, msg, args);
  fprintf(stderr, "\x1b[0m\n");
}

void Disable(std::set<std::string> scopes)
{
  _ignored = scopes;
}

[[noreturn]]
[[gnu::format(printf, 2, 3)]]
void Fail(const char *scope, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  XFmtV(DBGt_FAIL, scope, msg, args);
  va_end(args);
  exit(1);
}

[[gnu::format(printf, 2, 3)]]
void Err(const char *scope, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  XFmtV(DBGt_ERR, scope, msg, args);
  va_end(args);
}

[[gnu::format(printf, 2, 3)]]
void Warn(const char *scope, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  XFmtV(DBGt_WARN, scope, msg, args);
  va_end(args);
}

[[gnu::format(printf, 2, 3)]]
void Info(const char *scope, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  XFmtV(DBGt_INFO, scope, msg, args);
  va_end(args);
}

}
