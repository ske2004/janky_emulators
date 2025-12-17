#include "Bin.H"
#include "pce/Dbg.H"
#include <cstdlib>
#include <cstdio>
#include <memory>

PceBin::PceBin(CStr path)
{
  std::unique_ptr<FILE, int(*)(FILE*)> f(fopen(path, "rb"), fclose);

  if (f.get() == nullptr)
    throw "file not found";

  fseek(f.get(), 0, SEEK_END);
  sz=ftell(f.get());
  Dbg::Info("BIN", "ROM Size: %04llX", sz);
  dat=(U8*)malloc(sz);
  fseek(f.get(), 0, SEEK_SET);
  if (fread(dat, 1, sz, f.get())!=sz)
    throw "can't read";
}

PceBin::~PceBin()
{
  free(dat);
}

/* reads out of bounds by returning 0xFF */
auto PceBin::Get(U64 addr)->U8
{
  if (addr>=sz)
    return 0xFF;
  return dat[addr];
}

/* writes out of bounds by doing nothing */
auto PceBin::Set(U64 addr, U8 data)->void
{
  if (addr>=sz)
    return;
  dat[addr] = data;
}
