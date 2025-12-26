package main

import "core:log"
import "core:fmt"

Vdc_Addrs :: enum {
  Ctrl     = 0,
  Unknown1 = 1,
  Data_lo  = 2,
  Data_hi  = 3,
}

Vce_Addrs :: enum {
  Ctrl_lo = 0,
  Ctrl_hi = 1,
  /* 9 bits PPPPPPPPP */
  PalSelect_lo = 2,
  PalSelect_hi = 3,
  /* 9 bits GGGRRRBBB */
  PalColor_lo = 4,
  PalColor_hi = 5,
  Unknown6 = 6,
  Unknown7 = 7,
}

Psg_Addrs :: enum {
  Dummy, // todo
}

Timer_Addrs :: enum {
  ReloadVal = 0,
  Enable    = 1,
}

Joy_Addrs :: enum {
  Port,
}

Ictl_Addrs :: enum {
  Unknown0 = 0,
  Unknown1 = 1,
  Enabled  = 2,
  Pending  = 3,
}

Hwpage_Dst :: union {
  Vdc_Addrs,
  Vce_Addrs,
  Psg_Addrs,
  Timer_Addrs,
  Joy_Addrs,
  Ictl_Addrs,
}

hwpage_map :: #force_inline proc(addr: u16) -> Hwpage_Dst {
  if addr >= 0x0000 && addr <= 0x03FF do return Vdc_Addrs(addr%4)
  else if addr >= 0x0400 && addr <= 0x07FF do return Vce_Addrs(addr%8)
  else if addr >= 0x0800 && addr <= 0x0BFF do return Psg_Addrs(addr%1)
  else if addr >= 0x0C00 && addr <= 0x0FFF do return Timer_Addrs(addr%2)
  else if addr >= 0x1000 && addr <= 0x13FF do return Joy_Addrs(addr%1)
  else if addr >= 0x1400 && addr <= 0x17FF do return Ictl_Addrs(addr%4)
  else do return nil
}

hwpage_read :: proc(bus: ^Bus, addr: u16) -> u8 {
  bus_cycle(bus)

  switch v in hwpage_map(addr) {
  case Vdc_Addrs:   return vdc_read(bus, &bus.vdc, v)
  case Vce_Addrs:   return vce_read(&bus.vce, v)
  case Psg_Addrs:   return 0x00
  case Timer_Addrs: return timer_read(bus, &bus.timer, v)
  case Joy_Addrs:   return joy_read(&bus.joy)
  case Ictl_Addrs:  return ictl_read(bus, v)
  case nil:         return 0xFF
  }
  unreachable()
}

hwpage_write :: proc(bus: ^Bus, addr: u16, val: u8) {
  bus_cycle(bus)

  if addr >= 0x0800 && addr <= 0x17FF {
    bus.io_byte = val
  }

  switch v in hwpage_map(addr) {
  case Vdc_Addrs:   vdc_write(bus, &bus.vdc, v, val)
  case Vce_Addrs:   vce_write(&bus.vce, v, val)
  case Psg_Addrs:   log.warnf("psg write: %04X %02X", addr, val)
  case Timer_Addrs: timer_write(bus, &bus.timer, v, val)
  case Joy_Addrs:   joy_write(&bus.joy, val)
  case Ictl_Addrs:  ictl_write(bus, v, val)
  case nil:         return
  }
}