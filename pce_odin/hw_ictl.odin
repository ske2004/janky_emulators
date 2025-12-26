// interrupt controller
package main

ictl_write :: proc(bus: ^Bus, addr: ICtl_Addrs, val: u8) {
  log_instr_info("ictl write: %v %02X", addr, val)
  switch addr {
  case .Unknown0:
  case .Unknown1:
  case .Enabled:  bus.irq_disable = cast(IRQ_Reg)(val&0x7)
  case .Pending:  bus.irq_pending.timer = false
  }
}

ictl_read :: proc(bus: ^Bus, addr: ICtl_Addrs) -> u8 {
  log_instr_info("ictl read: %v", addr)
  switch addr {
  case .Unknown0: return 0 // ??
  case .Unknown1: return 0 // ??
  case .Enabled:  return (bus.io_byte&~7)|(cast(u8)bus.irq_disable&7)
  case .Pending:  return (bus.io_byte&~7)|(cast(u8)bus.irq_pending&7)
  }
  return 0
}
