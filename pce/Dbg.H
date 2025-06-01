/* debugging util */
#pragma once

#include "Basics.H"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#define DBGt_INFO  0 // normal log
#define DBGt_FAIL  1 // fatal error
#define DBGt_ERR   2 // error
#define DBGt_WARN  3 // warning

void DbgXFmtV(U64 dbgt, const char *msg, va_list args)
{
  CStr pfx = "";
  switch (dbgt) {
    case DBGt_INFO: pfx = "\x1b[34mINFO";   break;
    case DBGt_FAIL: pfx = "\x1b[31mFAIL";   break;
    case DBGt_ERR:  pfx = "\x1b[31mAAAA";   break;
    case DBGt_WARN: pfx = "\x1b[33mWARN";   break;
    default:        pfx = "\x1b[33m\?\?\?\?"; break; // stupid trigraphs v_v
  }

  fprintf(stderr, "%s \x1b[37m", pfx);
  vfprintf(stderr, msg, args);
  fprintf(stderr, "\x1b[0m\n");
}

[[noreturn]]
void DbgFail(const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  DbgXFmtV(DBGt_FAIL, msg, args);
  va_end(args);
  exit(1);
}

void DbgErr(const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  DbgXFmtV(DBGt_ERR, msg, args);
  va_end(args);
}

void DbgWarn(const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  DbgXFmtV(DBGt_WARN, msg, args);
  va_end(args);
}

void DbgInfo(const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  DbgXFmtV(DBGt_INFO, msg, args);
  va_end(args);
}
