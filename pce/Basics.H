/* basic defns */
#pragma once

#include <cstdint>
#define defexpr static constexpr

using U0   = void;
using I8   = int8_t;
using U8   = uint8_t;
using I16  = int16_t;
using U16  = uint16_t;
using I32  = int32_t;
using U32  = uint32_t;
using I64  = int64_t;
using U64  = uint64_t;
using Uxx  = size_t;
using Ixx  = intptr_t;
using Bool = bool;

using F32  = float;

using CStr = const char*;

template<Uxx N>
constexpr Uxx Length(char const (&)[N])
{
  return N-1;
}

/* inclusive range */
struct MemRange 
{
  Uxx from;
  Uxx to;
  constexpr MemRange(Uxx _from, Uxx _to) : from(_from), to(_to) {}
  constexpr auto Has(Uxx addr) const -> Bool {return addr>=from && addr<=to;}
};

struct MirroredMem : public MemRange 
{
  Uxx mirror_every;
  constexpr MirroredMem(Uxx _from, Uxx _to, Uxx _mirror_every) :
    MemRange(_from, _to), mirror_every(_mirror_every) {}
  constexpr auto Get(Uxx addr) const -> Uxx {return (addr-from)%mirror_every;}
};