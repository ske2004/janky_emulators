#include "HwPage.H"
#include "Dbg.H"

auto HwPage::Read(const U16 addr) -> U8
{
  Sub sub = Sub(addr>>10);
  if (sub >= Sub::_COUNT) {
    Dbg::Fail("HWP/R", "Read from invalid subpage");
  }

  Uxx mirror_every = MIRROR_EVERY[int(sub)];
  Uxx addr_mirrored = addr%mirror_every;

  switch (sub) {
  case Sub::VDC:
    return _vdc.Read(Vdc::Adr(addr_mirrored));
  case Sub::VCE:
    return _vce.Read(Vce::Adr(addr_mirrored));
  case Sub::TIMER:
    /* timer value */
    return (_timer.value_current&0x7F)|(_buffer_byte|0x80);
  case Sub::ICTL:
    if (addr_mirrored == 2) {
      return (_irq.enabled&0x7)|(_buffer_byte&~0x7);
    } else if (addr_mirrored == 3) {
      return (_irq.pending&0x7)|(_buffer_byte&~0x7);
    } 
    return _buffer_byte;
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

auto HwPage::Write(U16 addr, U8 value) -> U0
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
  case Sub::VDC:
    _vdc.Write(Vdc::Adr(addr_mirrored), value);
    return;
  case Sub::VCE:
    _vce.Write(Vce::Adr(addr_mirrored), value);
    return;
  case Sub::TIMER:
    /* timer value */
    if (addr_mirrored == 0) {
      _timer.value_reload = Uxx(value&0x7F)+1;
    } else if (addr_mirrored == 1) {
      _timer.enabled = value&1;
      _timer.value_current = _timer.value_reload;
    }
    return;
  case Sub::ICTL:
    if (addr_mirrored == 2) {
      _irq.enabled = value&0x7;
    } else if (addr_mirrored == 3) {
      _irq.Ack(Irq::TIMER);
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

auto HwPage::Cycle(Clk &clk) -> U0
{
  if (clk.counter%4==0) {
    _vdc.Cycle();
  }

  if (clk.counter%3==0) {
    _timer.clocks++;
    if (_timer.clocks%1024==0) {
      // Dbg::Info("HWP/C", "Timer tick!");
      if (_timer.enabled) {
        if (_timer.value_current==0) {
          _irq.Request(Irq::TIMER);
          _timer.value_current = _timer.value_reload;
        } else {
          _timer.value_current -= 1;
        }
      } 
    }
  }
}
