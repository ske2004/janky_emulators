/* PC-Engine phys memory map, denoted by pg number.
 * PC-Engine uses a 21bit phys memory bus. So, do bsr 13 to get pg number.
 *
 * $00:$7F ROM
 * $80:$F7 Unused (ret $FF)
 * $F8:$FB RAM (only 1 pg, $2000 bytes, rest is mirrored)
 * $FC:$FE Unused (ret $FF)
 * $FF:$FF Hardware pg [see HwPage.H]
 */

/* TIMING
 * 21.47727MHz Master clock
 * 7.16                 CPU (high speed)
 * 1.78                 CPU (low speed)
 * 7.16                 Timer
 * 10.8 or 7.16 or 5.36 VCE/VDC
 */

#pragma once

#include "Basics.H"
#include "Bin.H"
#include "Hu6.H"
#include "Arr.H"
#include "HwPage.H"
#include "Vce.H"
#include "Vdc.H"
#include "PceOut.H"

struct Pce : IBus
{
  PceBin &rom;
  Clk clk;
  Hu6 cpu;
  Vce vce;
  Vdc vdc;
  Irq irq;
  HwPage hwpage;
  Arr<U8, 0x2000> ram; // TODO: what is init ram state?

  Pce(PceBin &rom, PceOut &out);

  auto Read(U32 addr) -> U8 override;
  auto Write(U32 addr, U8 data) -> U0 override;
  auto Run()->U0;
};

