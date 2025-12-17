#include "Basics.H"

int PopCnt32(U32 v)
{
  #ifdef _MSC_VER
  return __popcnt(v);
  #else
  return __builtin_popcount(v);
  #endif
}

int Ctz32(U32 v)
{
  #ifdef _MSC_VER
  unsigned long index;
  _BitScanForward(&index, v);
  return index;
  #else
  return __builtin_ctz(v);
  #endif
}