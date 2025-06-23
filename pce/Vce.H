#pragma once

#include "Basics.H"
#include "Dbg.H"
struct Vce
{
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
    case Adr::CTRL: Dbg::Info("VCE: Write to CTRL: $%02X", data); break;
    default: Dbg::Info("VCE: Write to unhandled %d", adr); break;
    }
  }

  auto Read(Adr adr) -> U8 
  {
    switch (adr) {
    default: Dbg::Info("VCE: Read from unhandled %d", adr); break;
    }

    return 0;
  }
};