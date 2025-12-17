#pragma once
#include "Basics.H"

struct IBus 
{
  virtual auto Read(U32 addr) -> U8 = 0;
  virtual auto Write(U32 addr, U8 data) -> U0 = 0;
};

