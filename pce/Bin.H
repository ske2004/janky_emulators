#pragma once
#include "Basics.H"
#include <cstdlib>
#include <cstdio>

struct Bin
{
  U64 sz;
  U8 dat[];
};

auto BinNew(U64 sz)->Bin*
{
  Bin *bin=(Bin*)malloc(sizeof(Bin)+sz);
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
  if (fread(bin->dat, 1, sz, f)!=sz)
    return nullptr;
  return bin;
}

auto BinWrite(CStr path, Bin *bin)->Bool
{
  FILE *f=fopen(path, "wb");
  if (!f)
    return false;
  if (fwrite(bin->dat, 0, bin->sz, f) != bin->sz)
    return false;
  return true;
}
