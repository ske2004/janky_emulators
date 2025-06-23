#pragma once

#include "Basics.H"
#include "Dbg.H"

struct Vdc
{
  enum class Reg
  {
    MAWR    =0, /* VDC memory write */
    MARR    =1, /* VDC memory read */
    VRW     =2, /* VRAM read/write */
    UNUSED0 =3,
    UNUSED1 =4,
    CR      =5, /* control register */
    RCR     =6, /* raster compare register */
    UNUSED2 =7,
    UNUSED3 =8,
    MWR     =9, /* memory width register */
  };

  enum class Adr
  {
    REG_SEL =0, /* select register */
    UNUSED0 =1,
    DATA_LO =2,
    DATA_HI =3,
  };

  auto Write(Adr adr, U8 data)
  {
    switch (adr) {
    case Adr::REG_SEL: Dbg::Info("VDC: Write to REG_SEL: $%02X", data); break;
    default: Dbg::Info("VDC: Write to unhandled %d", adr); break;
    }
  }

  auto Read(Adr adr) -> U8
  {
    switch (adr) {
    default: Dbg::Info("VDC: Read from unhandled %d", adr); break;
    }
    return 0xFF;
  }
};