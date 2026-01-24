package main

Timer :: struct {
  enabled: bool,
  reload: u8,
  counter: u8,
  clocks: uint,
}

timer_read :: proc(bus: ^Bus, timer: ^Timer, addr: Timer_Addrs) -> u8 {
  switch addr {
  case .ReloadVal: return (timer.reload&0x7F)|(bus.io_byte&0x80) 
  case .Enable:    return (u8(timer.enabled)&0x01)|(bus.io_byte&0x80)
  }
  unreachable()
}

timer_write :: proc(bus: ^Bus, timer: ^Timer, addr: Timer_Addrs, val: u8) {
  switch addr {
  case .ReloadVal: timer.reload = val&0x7F // +1 added later!!!
  case .Enable:    timer.enabled = bool(val&1)
  }
}

timer_cycle :: proc(bus: ^Bus, timer: ^Timer) {
  timer.clocks += 1
  
  if timer.clocks%1024 == 0 {
    if timer.enabled {
      if timer.counter == 0 {
        timer.counter = timer.reload+1
        bus_irq(bus, .Timer)
      } else {
        timer.counter -= 1
      }
    }
  }
}