package main

Timer :: struct {
  enabled: bool,
  reload: u8,
  counter: u8,
  clocks: uint,
}

timer_read :: proc(bus: ^Bus, using timer: ^Timer, addr: Timer_Addrs) -> u8 {
  switch addr {
  case .ReloadVal: return (reload&0x7F)|(bus.io_byte&0x80) 
  case .Enable:    return (u8(enabled)&0x01)|(bus.io_byte&0x80)
  }
  unreachable()
}

timer_write :: proc(bus: ^Bus, using timer: ^Timer, addr: Timer_Addrs, val: u8) {
  switch addr {
  case .ReloadVal: reload = val&0x7F // +1 added later!!!
  case .Enable:    enabled = bool(val&1)
  }
}

timer_cycle :: proc(bus: ^Bus, using timer: ^Timer) {
  clocks += 1
  
  if clocks%1024 == 0 {
    if enabled {
      if counter == 0 {
        counter = reload+1
        bus_irq(bus, .Timer)
      } else {
        counter -= 1
      }
    }
  }
}