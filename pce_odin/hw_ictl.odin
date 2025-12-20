// interrupt controller
package main

ictl_write :: proc(bus: ^Bus, addr: IctlAddrs, val: u8) {
  switch addr {
  case .Unknown0:
  case .Unknown1:
  case .Enabled:  bus.irq_enable = cast(IrqReg)(val&0x7)
  case .Pending:  bus.irq_pending.timer = false
  }
}

ictl_read :: proc(bus: ^Bus, addr: IctlAddrs) -> u8 {
  switch addr {
  case .Unknown0: return 0 // ??
  case .Unknown1: return 0 // ??
  case .Enabled:  return (bus.io_byte&~7)|(cast(u8)bus.irq_enable&7)
  case .Pending:  return (bus.io_byte&~7)|(cast(u8)bus.irq_pending&7)
  }
  return 0
}
