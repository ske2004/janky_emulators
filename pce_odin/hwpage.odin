package main

import "core:log"

VdcAddrs :: enum {
  Ctrl     = 0,
  Unknown1 = 1,
  Data_lo  = 2,
  Data_hi  = 3,
}

VceAddrs :: enum {
  Ctrl = 0,
  /* 9 bits PPPPPPPPP */
  PalSelect_lo = 1,
  PalSelect_hi = 2,
  /* 9 bits GGGRRRBBB */
  PalColor_lo = 3,
  PalColor_hi = 4,
  Unknown5 = 5,
  Unknown6 = 6,
  Unknown7 = 7,
}

PsgAddrs :: enum {
  Dummy // todo
}

TimerAddrs :: enum {
  ReloadVal = 0,
  Enable    = 1
}

IoAddrs :: enum {
  Dummy
}

IctlAddrs :: enum {
  Unknown0 = 0,
  Unknown1 = 1,
  Enabled  = 2,
  Pending  = 3
}

HwpageDst :: union {
  VdcAddrs,
  VceAddrs,
  PsgAddrs,
  TimerAddrs,
  IoAddrs,
  IctlAddrs,
}

hwpage_map :: #force_inline proc(addr: u16) -> HwpageDst {
  if addr >= 0x0000 && addr <= 0x03FF do return VdcAddrs(addr%4)
  else if addr >= 0x0400 && addr <= 0x07FF do return VceAddrs(addr%8)
  else if addr >= 0x0800 && addr <= 0x0BFF do return PsgAddrs(addr%1)
  else if addr >= 0x0C00 && addr <= 0x0FFF do return TimerAddrs(addr%2)
  else if addr >= 0x1000 && addr <= 0x13FF do return IoAddrs(addr%1)
  else if addr >= 0x1400 && addr <= 0x17FF do return IctlAddrs(addr%4)
  else do return nil
}

hwpage_read :: proc(bus: ^Bus, addr: u16) -> u8 {
  switch v in hwpage_map(addr) {
    case VdcAddrs:   return vdc_read(bus, &bus.vdc, v)
    case VceAddrs:   return vce_read(&bus.vce, v)
    case PsgAddrs:   unimplemented("read psgaddrs")
    case TimerAddrs: return timer_read(bus, &bus.timer, v)
    case IoAddrs:    unimplemented("read ioaddrs")
    case IctlAddrs:  return ictl_read(bus, v)
    case nil:        return 0xFF
  }
  unreachable()
}

hwpage_write :: proc(bus: ^Bus, addr: u16, val: u8) {
  if addr >= 0x0800 && addr <= 0x17FF {
    bus.io_byte = val
  }

  switch v in hwpage_map(addr) {
    case VdcAddrs:   vdc_write(bus, &bus.vdc, v, val)
    case VceAddrs:   vce_write(&bus.vce, v, val)
    case PsgAddrs:   unimplemented("write psgaddrs")
    case TimerAddrs: timer_write(bus, &bus.timer, v, val)
    case IoAddrs:    unimplemented("write ioaddrs")
    case IctlAddrs:  ictl_write(bus, v, val)
    case nil:        return
  }
}