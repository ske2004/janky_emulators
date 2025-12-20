package main

import "core:log"

Irq :: enum u8 {
  Irq2,
  Irq1,
  Timer,
}

IrqReg :: bit_field u8 {
  irq2: bool  | 1,
  irq1: bool  | 1, 
  timer: bool | 1, 
}

Bus :: struct {
  rom: []byte,
  ram: [0x2000]u8,
  clocks: uint,

  irq_pending: IrqReg,
  irq_enable: IrqReg,

  io_byte: u8,

  timer: Timer,

  vblank_occured: bool,
  screen: [256*224]Rgb333,

  vdc: Vdc,
  vce: Vce,
}

@(no_instrumentation=true)
rom_read :: proc(rom: []u8, addr: u32) -> u8 {
  return (int(addr) >= len(rom)) ? 0xFF : rom[addr]
}

bus_init :: proc(rom: []u8) -> Bus {
  return Bus{rom = rom}
}

bus_read_u8 :: proc(bus: ^Bus, addr: u32) -> u8 {
  page := addr>>BANK_SHIFT
  mapped_addr := cast(u16)(addr&BANK_MASK)
  
  // log.infof("read: addr=%06X page=%02X maddr=%04X", addr, page, mapped_addr)

  if page>=0x00 && page<=0x7F do return rom_read(bus.rom, addr)
  else if page>=0xF8 && page<=0xFB do return bus.ram[mapped_addr]
  else if page==0xFF do return hwpage_read(bus, mapped_addr)

  return 0xFF
}

bus_write_u8 :: proc(bus: ^Bus, addr: u32, value: u8) {
  page := addr>>BANK_SHIFT
  mapped_addr := cast(u16)(addr&BANK_MASK)
  
  // log.infof("write: addr=%06X page=%02X maddr:%04X val:%02X", addr, page, mapped_addr, value)

  if page>=0x00 && page<=0x7F do return
  else if page>=0xF8 && page<=0xFB do bus.ram[mapped_addr] = value
  else do hwpage_write(bus, mapped_addr, value)
}

bus_cycle :: proc(bus: ^Bus) {
  bus.clocks += 1
  timer_cycle(bus, &bus.timer)
  vdc_cycle(bus, &bus.vdc)
}

bus_irq :: proc(bus: ^Bus, irq: Irq) {
  bit := cast(u8)1<<irq
  if (cast(u8)bus.irq_enable & bit) > 0 {
    bus.irq_pending |= transmute(IrqReg)bit
  }
}

