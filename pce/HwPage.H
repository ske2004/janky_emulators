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
#include "Dbg.H"
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
    U8 value_current;
    U8 value_reload;
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
  U8 _buffer_byte=0;
  U8 _ictl_enabled=0;
  U8 _ictl_pending=0;
  Timer _timer={};

  HwPage(Vce &vce, Vdc &vdc) : _vce(vce), _vdc(vdc) {}

  auto HasInterrupt(Interrupt i) -> Bool
  {
    return _ictl_enabled&(1<<U8(i));
  }

  auto SetInterrupt(Interrupt i) -> U0
  {
    _ictl_pending |= 1<<U8(i);
  }

  auto Read(const U16 addr) -> U8
  {
    Sub sub = Sub(addr>>10);
    if (sub >= Sub::_COUNT) {
      Dbg::Fail("HWP/R", "Read from invalid subpage");
    }

    Uxx mirror_every = MIRROR_EVERY[int(sub)];
    Uxx addr_mirrored = addr%mirror_every;

    switch (sub) {
    case Sub::VDC: return _vdc.Read(Vdc::Adr(addr_mirrored));
    case Sub::VCE: return _vce.Read(Vce::Adr(addr_mirrored));
    case Sub::TIMER:
      /* timer value */
      if (addr_mirrored == 0) {
        return (_timer.value_current&0x7F)|(_buffer_byte|0x80);
      } else if (addr_mirrored == 1) {
        return (int(_timer.enabled)&0x7F)|(_buffer_byte|0x80);
      }
      return 0xFF;
    case Sub::ICTL:
      if (addr_mirrored == 2) {
        return (_ictl_enabled&0x7)|(_buffer_byte&~0x7);
      } else if (addr_mirrored == 3) {
        return (_ictl_pending&0x7)|(_buffer_byte&~0x7);
      }
      return 0xFF;
    case Sub::PCG:
    case Sub::IO:
    case Sub::CDROM:
    case Sub::FF:
      Dbg::Err("HWP/R", "HwPage %d Read: TODO", sub);
      return 0xFF;
    default:
      [[assume(false)]];
      return 0xFF;
    } 
  }

  auto Write(U16 addr, U8 value) -> U0
  {
    Sub sub = Sub(addr>>10);
    if (sub >= Sub::_COUNT) {
      Dbg::Fail("HWP/W", "Write to invalid subpage");
    }

    if (IO_BUFFER.Has(addr)) {
      _buffer_byte = value;
    }

    Uxx mirror_every = MIRROR_EVERY[int(sub)];
    Uxx addr_mirrored = addr%mirror_every;

    switch (sub) {
    case Sub::VDC: _vdc.Write(Vdc::Adr(addr_mirrored), value); return;
    case Sub::VCE: _vce.Write(Vce::Adr(addr_mirrored), value); return;
    case Sub::TIMER:
      /* timer value */
      if (addr_mirrored == 0) {
        _timer.value_reload = (value&0x7F)+1;
      } else if (addr_mirrored == 1) {
        _timer.enabled = value&1;
        _timer.value_current = _timer.value_reload;
      }
      return;
    case Sub::ICTL:
      if (addr_mirrored == 2) {
        _ictl_enabled = value&0x7;
      } else if (addr_mirrored == 3) {
        /* TODO */
        _ictl_pending = 0;
      }
      return;
    case Sub::PCG: 
    case Sub::IO:
    case Sub::CDROM:
    case Sub::FF:
      Dbg::Err("HWP/W", "HwPage %d Write: TODO", sub);
      return;
    default:
      [[assume(false)]];
      return;
    }
  }

  auto Cycle(Clk &clk)
  {
    if (clk.counter%3==0) {
      _timer.clocks++;
      if (_timer.clocks%1024==0) {
        Dbg::Info("HWP/C", "Timer tick!");
        if (_timer.enabled) {
          if (_timer.value_current==0) {
            _timer.value_current = _timer.value_reload;
          } else {
            _timer.value_current -= 1;
          }
        } 
      }
    }
  }
};