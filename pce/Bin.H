/* bin data manipulation */
#pragma once

#include "Basics.H"
#include "Dbg.H"
#include <cstdlib>
#include <cstdio>

struct Bin
{
  U64 sz;
  U8 dat[0];
};

auto BinNew(U64 sz)->Bin*
{
  DbgInfo("BIN: new, size: $%llx", sz);
  auto bin=(Bin*)malloc(sizeof(Bin)+sz);
  bin->sz=sz;
  return bin;
}

auto BinDel(Bin *bin)
{
  free(bin);
}

auto BinRead(CStr path)->Bin*
{
  FILE *f=fopen(path, "rb");
  if (!f)
    return nullptr;
  fseek(f, 0, SEEK_END);
  U64 sz=ftell(f);
  fseek(f, 0, SEEK_SET);
  Bin *bin=BinNew(sz);
  if (fread(bin->dat, 1, sz, f)!=sz) {
    fclose(f);
    BinDel(bin);
    return nullptr;
  }
  fclose(f);
  return bin;
}

auto BinWrite(CStr path, Bin *bin)->Bool
{
  FILE *f=fopen(path, "wb");
  if (!f)
    return false;
  if (fwrite(bin->dat, 0, bin->sz, f) != bin->sz) {
    fclose(f);
    return false;
  }
  fclose(f);
  return true;
}

/* reads out of bounds by returning 0xFF */
auto BinGetOob(Bin *bin, U64 addr)->U8
{
  if (addr>=bin->sz)
    return 0xFF;
  return bin->dat[addr];
}

/* writes out of bounds by doing nothing */
auto BinSetOob(Bin *bin, U64 addr, U8 data)->Bool
{
  if (addr>=bin->sz)
    return false;
  bin->dat[addr] = data;
  return true;
}
