package main

import "core:log"
import "core:fmt"

VDC_Addrs :: enum {
  Ctrl     = 0,
  Unknown1 = 1,
  Data_lo  = 2,
  Data_hi  = 3,
}

VCE_Addrs :: enum {
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

PSG_Addrs :: enum {
  Dummy, // todo
}

Timer_Addrs :: enum {
  ReloadVal = 0,
  Enable    = 1,
}

Joy_Addrs :: enum {
  Port,
}

ICtl_Addrs :: enum {
  Unknown0 = 0,
  Unknown1 = 1,
  Enabled  = 2,
  Pending  = 3,
}

HW_Page_Dst :: union {
  VDC_Addrs,
  VCE_Addrs,
  PSG_Addrs,
  Timer_Addrs,
  Joy_Addrs,
  ICtl_Addrs,
}

hwpage_map :: #force_inline proc(addr: u16) -> HW_Page_Dst {
  if addr >= 0x0000 && addr <= 0x03FF do return VDC_Addrs(addr%4)
  else if addr >= 0x0400 && addr <= 0x07FF do return VCE_Addrs(addr%8)
  else if addr >= 0x0800 && addr <= 0x0BFF do return PSG_Addrs(addr%1)
  else if addr >= 0x0C00 && addr <= 0x0FFF do return Timer_Addrs(addr%2)
  else if addr >= 0x1000 && addr <= 0x13FF do return Joy_Addrs(addr%1)
  else if addr >= 0x1400 && addr <= 0x17FF do return ICtl_Addrs(addr%4)
  else do return nil
}

hwpage_read :: proc(bus: ^Bus, addr: u16) -> (val: u8) {
  switch v in hwpage_map(addr) {
  case VDC_Addrs:   bus_cycle(bus); val = vdc_read(bus, &bus.vdc, v)
  case VCE_Addrs:   bus_cycle(bus); val = vce_read(&bus.vce, v)
  case PSG_Addrs:   val = bus.io_byte
  case Timer_Addrs: val = timer_read(bus, &bus.timer, v)
  case Joy_Addrs:   val = joy_read(&bus.joy)
  case ICtl_Addrs:  val = ictl_read(bus, v)
  case nil:         val = 0xFF
  }

  if addr >= 0x0800 && addr <= 0x17FF {
    bus.io_byte = val
  }

  return
}

hwpage_write :: proc(bus: ^Bus, addr: u16, val: u8) {
  if addr >= 0x0800 && addr <= 0x17FF {
    bus.io_byte = val
  }

  switch v in hwpage_map(addr) {
  case VDC_Addrs:   bus_cycle(bus); vdc_write(bus, &bus.vdc, v, val)
  case VCE_Addrs:   bus_cycle(bus); vce_write(&bus.vce, v, val)
  case PSG_Addrs:   log.warnf("psg write: %04X %02X", addr, val)
  case Timer_Addrs: timer_write(bus, &bus.timer, v, val)
  case Joy_Addrs:   joy_write(&bus.joy, val)
  case ICtl_Addrs:  ictl_write(bus, v, val)
  case nil:         return
  }
}