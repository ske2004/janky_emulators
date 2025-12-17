#pragma once

#include "Basics.H"
#include "Dbg.H"
#include "PceOut.H"
#include "Vce.H"
#include "Vram.H"
#include "Irq.H"

namespace _vdc {
  static const char *adr_names[] = {
    "REG_SEL",
    "UNUSED0",
    "DATA_LO",
    "DATA_HI",
  };

  static const char *reg_names[] = {
    "MAWR",
    "MARR",
    "VRW",
    "UNUSED0",
    "UNUSED1",
    "CR",
    "RCR",
    "SCROLLX",
    "SCROLLY",
    "MWR",
    "HSYNC",
    "HDISP",
    "VSYNC",
    "VDISP",
    "VEND",
    "DMACTRL",
    "DMASRC",
    "DMADST",
    "DMALEN",
    "DMASPR",
  };
}

static auto PiecewiseWrite16(U16 &dest, U8 value, Bool lo) -> void
{
  dest = lo ? ((dest & 0xFF00) | value) : ((dest & 0xFF) | (U16)value << 8);
}

struct Vdc
{
  enum class Reg
  {
    MAWR   , /* VDC memory write */
    MARR   , /* VDC memory read */
    VRW    , /* VRAM read/write */
    UNUSED0,
    UNUSED1,
    CR     , /* control register */
    RCR    , /* raster compare register */
    SCROLLX,
    SCROLLY,
    MWR    , /* memory width register */
    HSYNC  ,
    HDISP  ,
    VSYNC  ,
    VDISP  ,
    VEND   ,
    DMACTRL,
    DMASRC ,
    DMADST ,
    DMALEN ,
    DMASPR ,
    _COUNT
  };

  enum class Latch
  {
    
  };

  enum class Adr
  {
    REG_SEL, /* select register */
    UNUSED0,
    DATA_LO,
    DATA_HI,
  };

  U8 selected_register = 0;

  U8 rwmode = 0; /* 0 = r ; 1 = w */
  U16 addr = 0;

  Vce &vce;
  Vce::Color screen[256*224];

  U16 x=0, y=0;
  PceOut &out;

  U16 satb_dma = 0;
  Bool satb_dma_next_vblank = false;

  Vram vram {};

  Irq &irq;

  Vdc(Vce &vce, PceOut &out, Irq &irq) : vce(vce), out(out), irq(irq) {}

  void PlotDot(int x, int y, Vce::Color value);
  // not how it works
  void FillScreen();
  auto Cycle() -> U0;
  auto RegWrite(Reg reg, U8 data, Bool lo) -> void;
  auto Write(Adr adr, U8 data) -> void;
  auto Read(Adr adr) -> U8;
};
