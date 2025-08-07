#pragma once

#include "Basics.H"
#include "Dbg.H"


struct Vce
{
  /* xxxxxxxBBBGGGRRR */
  using Color=U16;

  /* 000=bg 100=spr */
  Color entries[512];

  enum class Adr {
    CTRL      =0,
    UNUSED0   =1,
    COLTBL_ADDR_LO =2,
    COLTBL_ADDR_HI =3,

    /* 9 bits GGGRRRBBB */
    COLTBL_DATA_LO =4,
    COLTBL_DATA_HI =5,
    UNUSED1        =6,
    UNUSED2        =7
  };

  auto Write(Adr adr, U8 data) -> void
  {
    switch (adr) {
    case Adr::CTRL: Dbg::Info("VCE/W", "Write to CTRL: $%02X", data); break;
    default: Dbg::Info("VCE/W", "Write to unhandled %d", adr); break;
    }
  }

  auto Read(Adr adr) -> U8 
  {
    switch (adr) {
    case Adr::CTRL: return 0xFF; break;
    default: Dbg::Info("VCE/R", "Read from unhandled %d", adr); break;
    }

    return 0;
  }
};