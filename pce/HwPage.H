/* Hardware page!
 * 
 * n = actual bytes, mirrored
 * 
 * $0000:$03FF[4] VDC
 * $0400:$07FF[8] VCE
 * $0800:$0BFF[1] PSG buffer
 * $0C00:$0FFF[2] Timer
 * $1000:$13FF[1] IO port
 * $1400:$17FF[4] Interrupt control
 * $1800:$1BFF[1] Returns $FF (CDROM)
 * $1C00:$1FFF[1] Returns $FF
 * 
 * Last byte written to $0800:$17FF is saved in internal buffer.
 */
#pragma once

#include "Clk.H"
#include "Basics.H"
#include "Vce.H"
#include "Vdc.H"

struct HwPage
{
  enum class Sub
  {
    VDC, VCE, PCG, TIMER, IO, ICTL, CDROM, FF, _COUNT
  };

  enum class Interrupt
  {
    IRQ2,
    IRQ1,
    IRQ0
  };

  struct Timer 
  {
    Uxx clocks;
    Uxx value_current;
    Uxx value_reload;
    Bool enabled;
  };

  defexpr Uxx MIRROR_EVERY[8] = {
    4, /* VDC */
    8, /* VCE */
    1, /* PCG */
    2, /* TIMER */
    1, /* IO */
    4, /* INTERRUPT CTRL */
    1, /* CDROM */
    1, /* FF */
  };

  defexpr MemRange IO_BUFFER {0x0800, 0x17FF};

  Vce &_vce;
  Vdc &_vdc;
  Irq _irq;
  U8 _buffer_byte=0;
  Timer _timer={};

  HwPage(Vce &vce, Vdc &vdc, Irq &irq) : _vce(vce), _vdc(vdc), _irq(irq) {}

  auto Read(const U16 addr) -> U8;
  auto Write(U16 addr, U8 value) -> U0;
  auto Cycle(Clk &clk) -> U0;
};