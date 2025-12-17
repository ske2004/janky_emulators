#pragma once

#include "Basics.H"
#include "Dbg.H"

struct Vce
{
  /* xxxxxxxBBBGGGRRR */
  using Color=U16;

  /* $000=bg $100=spr */
  Color entries[0x200];

  U16 pal_index;

  enum class Adr {
    CTRL         =0,
    /* 9 bits PPPPPPPPP */
    PALSELECT_LO =1,
    PALSELECT_HI =2,
    /* 9 bits GGGRRRBBB */
    PALCOLOR_LO  =3,
    PALCOLOR_HI  =4,
  };

  auto Write(Adr adr, U8 data) -> void;
  auto Read(Adr adr) -> U8;
};