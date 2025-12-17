/* bin data manipulation */
#pragma once

#include "Basics.H"

struct Bin
{
  U64 sz;
  U8 *dat;
};

class PceBin : private Bin
{
public:
  PceBin(CStr path);
  ~PceBin();

  auto Get(U64 addr)->U8;
  auto Set(U64 addr, U8 data)->void;
};

