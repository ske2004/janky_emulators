// huc6280 based on cmos
package main

import "core:fmt"
import "core:log"

Block_Transfer_Op :: enum {
  NONE,
  INC,
  DEC,
  INCDEC,
}

Block_Transfer_Reg :: struct {
  op: Block_Transfer_Op,
  value: u16,
  state: bool,
}

CPU_Flags :: bit_field u8 {
  car: bool | 1, // carry
  zer: bool | 1, // zero
  int: bool | 1, // interrupt
  dec: bool | 1, // decimal
  brk: bool | 1, // break
  mem: bool | 1, // memory
  ovf: bool | 1, // overflow
  neg: bool | 1, // negative
}

Vecs :: struct #packed {
  irq2:  u16le, // $FFF6
  irq1:  u16le, // $FFF8
  timer: u16le, // $FFFA
  nmi:   u16le, // $FFFC
  reset: u16le, // $FFFE
}

Opc_Info :: struct {
  instr: Instr,
  adr: Adr_Mode,
  ref_cyc: uint,// refence cyles, can be 0 if unknonw
  extra: u8,    // for these instruction with numbers c:
}

CPU :: struct {
  a, x, y, sp: u8,
  avr: u16,
  pc: u16,
  p: CPU_Flags,
  mpr: [8]u8,
  bus: ^Bus,
  fast: bool,// csh and csl
  opc_tbl: [256]Opc_Info,
}

BANK_START :: 7
BANK_MASK  :: 0x1FFF
BANK_SHIFT :: 13

ZPG_START :: 0x2000
STK_START :: 0x2100

IRQ2_START  :: 0xFFF6
IRQ1_START  :: 0xFFF8
TIMER_START :: 0xFFFA
NMI_START   :: 0xFFFC
RESET_START :: 0xFFFE


@(no_instrumentation=true) 
bank_sel :: #force_inline proc(bank: u16) -> u16 {
  assert(bank <= 7)
  return bank << BANK_SHIFT
}

@(no_instrumentation=true) 
addr_bank :: #force_inline proc(addr: u16) -> u16 {
  return addr >> BANK_SHIFT
}

@(no_instrumentation=true) 
cpu_mem_map :: #force_inline proc(cpu: ^CPU, addr: u16) -> u32 {
  low_addr := addr&BANK_MASK
  return (u32(cpu.mpr[addr_bank(addr)]) << BANK_SHIFT) | u32(low_addr)
}

@(no_instrumentation=true) 
cpu_cycle :: #force_inline proc(cpu: ^CPU) {
  if cpu.fast {
    bus_cycle(cpu.bus)
  } else {
    bus_cycle(cpu.bus)
    bus_cycle(cpu.bus)
    bus_cycle(cpu.bus)
    bus_cycle(cpu.bus)
  }
}

@(no_instrumentation=true) 
cpu_read_u8 :: #force_inline proc(cpu: ^CPU, addr: u16) -> u8 {
  value := bus_read_u8(cpu.bus, cpu_mem_map(cpu, addr))
  cpu_cycle(cpu)
  return value
}

@(no_instrumentation=true) 
cpu_read_u16 :: #force_inline proc(cpu: ^CPU, addr: u16) -> u16 {
  lo := cast(u16)bus_read_u8(cpu.bus, cpu_mem_map(cpu, addr))
  cpu_cycle(cpu)
  hi := cast(u16)bus_read_u8(cpu.bus, cpu_mem_map(cpu, addr+1))
  cpu_cycle(cpu)
  return (hi << 8) | lo
}

@(no_instrumentation=true) 
cpu_read_u16_zpg :: #force_inline proc(cpu: ^CPU, addr: u8) -> u16 {
  lo := cast(u16)bus_read_u8(cpu.bus, cpu_mem_map(cpu, ZPG_START|cast(u16)(addr)))
  cpu_cycle(cpu)
  hi := cast(u16)bus_read_u8(cpu.bus, cpu_mem_map(cpu, ZPG_START|cast(u16)(addr+1)))
  cpu_cycle(cpu)
  return (hi << 8) | lo
}

@(no_instrumentation=true) 
cpu_write_u8 :: #force_inline proc(cpu: ^CPU, addr: u16, val: u8) {
  bus_write_u8(cpu.bus, cpu_mem_map(cpu, addr), val)
  cpu_cycle(cpu)
}

cpu_read_pc_u8 :: #force_inline proc(cpu: ^CPU) -> u8 {
  value := bus_read_u8(cpu.bus, cpu_mem_map(cpu, cpu.pc))
  cpu_cycle(cpu)
  cpu.pc += 1
  return value
}

cpu_stk_push_u8 :: #force_inline proc(cpu: ^CPU, val: u8) {
  cpu_read_u8(cpu, STK_START|cast(u16)cpu.sp) // dummy
  cpu_write_u8(cpu, STK_START|cast(u16)cpu.sp, val)
  cpu.sp -= 1
}

cpu_stk_push_u16 :: #force_inline proc(cpu: ^CPU, val: u16) {
  cpu_stk_push_u8(cpu, cast(u8)(val>>8))
  cpu_stk_push_u8(cpu, cast(u8)(val&0xFF))
}

cpu_stk_write_u8 :: #force_inline proc(cpu: ^CPU, val: u8) {
  // todo: do i need dummy here?
  cpu_write_u8(cpu, STK_START|cast(u16)cpu.sp, val)
}

cpu_stk_pop_u8 :: #force_inline proc(cpu: ^CPU) -> u8 {
  cpu.sp += 1
  // and here??
  cpu_read_u8(cpu, STK_START|cast(u16)cpu.sp) // dummy
  return cpu_read_u8(cpu, STK_START|cast(u16)cpu.sp)
}

cpu_stk_pop_u16 :: #force_inline proc(cpu: ^CPU) -> u16 {
  lo := cast(u16)cpu_stk_pop_u8(cpu)
  hi := cast(u16)cpu_stk_pop_u8(cpu)
  return (hi << 8) | lo
}

cpu_stk_read_u8 :: #force_inline proc(cpu: ^CPU) -> u8 {
  // and here??
  return cpu_read_u8(cpu, STK_START|cast(u16)cpu.sp)
}

cpu_dummy_read :: #force_inline proc(cpu: ^CPU) {
  cpu_read_u8(cpu, cpu.pc)
}

cpu_read_pc_u16 :: #force_inline proc(cpu: ^CPU) -> u16 {
  lo := cast(u16)bus_read_u8(cpu.bus, cpu_mem_map(cpu, cpu.pc))
  hi := cast(u16)bus_read_u8(cpu.bus, cpu_mem_map(cpu, cpu.pc+1))
  cpu.pc += 2
  return (hi << 8) | lo
}

cpu_set_nz :: #force_inline proc(cpu: ^CPU, value: u8) {
  cpu.p.neg = (transmute(i8)value) < 0
  cpu.p.zer = value == 0
}

@(no_instrumentation) 
update_block_transfer_reg :: #force_inline proc(reg: ^Block_Transfer_Reg) {
  switch reg.op {
  case .NONE:
  case .INC: reg.value += 1
  case .DEC: reg.value -= 1
  case .INCDEC: if reg.state { reg.value -= 1 }
                else { reg.value += 1 }
                reg.state = !reg.state
  }
}

block_transfer :: proc(cpu: ^CPU, srcop, dstop: Block_Transfer_Op) {
  clock_start := cpu.bus.clocks
  src := Block_Transfer_Reg{srcop, cpu_read_pc_u16(cpu), false}
  dst := Block_Transfer_Reg{dstop, cpu_read_pc_u16(cpu), false}
  len := cpu_read_pc_u16(cpu)
 
  log_instr_info("BLK src=%s dst=%s: src:%04X (%06X), dst:%04X, len:%04X", srcop, dstop, src.value, cpu_mem_map(cpu, src.value), dst.value, len)

  cpu_stk_push_u8(cpu, cpu.y)
  cpu_stk_push_u8(cpu, cpu.a)
  cpu_stk_write_u8(cpu, cpu.x)

  clock_tran_start := cpu.bus.clocks
  
  for {
    cpu_write_u8(cpu, dst.value, cpu_read_u8(cpu, src.value))
    update_block_transfer_reg(&src)
    update_block_transfer_reg(&dst)
    len -= 1
    if len <= 0 do break
  }
  
  clock_tran_end := cpu.bus.clocks

  cpu.x = cpu_stk_read_u8(cpu)
  cpu.a = cpu_stk_pop_u8(cpu)
  cpu.y = cpu_stk_pop_u8(cpu)

  cyc_total := cpu.bus.clocks-clock_start
  cyc_tran := clock_tran_end-clock_tran_start
  cyc_notran := cyc_total-cyc_tran+1 // account for opc cycle

  log_instr_info("BLK: cyctot:%d, cyctrn:%d, cycntr:%d", cyc_total, cyc_tran, cyc_notran)
}

cpu_enter_irq :: proc(cpu: ^CPU, addr: u16) {
  cpu_stk_push_u16(cpu, cpu.pc)
  cpu_stk_push_u8(cpu, cast(u8)cpu.p)

  cpu.p.int = true

  cpu.pc = addr
}

cpu_check_irq :: proc(cpu: ^CPU) {
  if cpu.p.int == false {
    if cpu.bus.irq_pending.irq2 && !cpu.bus.irq_disable.irq2 {
      cpu.bus.irq_pending.irq2 = false
      cpu_enter_irq(cpu, cpu_read_u16(cpu, IRQ2_START))
    } else if cpu.bus.irq_pending.irq1 && !cpu.bus.irq_disable.irq1 {
      cpu.bus.irq_pending.irq1 = false
      log_instr_info("intercepting vblank")
      cpu_enter_irq(cpu, cpu_read_u16(cpu, IRQ1_START))
    } else if cpu.bus.irq_pending.timer && !cpu.bus.irq_disable.timer {
      log_instr_info("intercepting timer irq")
      cpu.bus.irq_pending.timer= false
      cpu_enter_irq(cpu, cpu_read_u16(cpu, TIMER_START))
    }
  }
}

cpu_init :: proc(bus: ^Bus) -> CPU {
  return {
    p = {int = true, brk = true},
    pc = cast(u16)bus.rom[0x1FFE] | cast(u16)bus.rom[0x1FFF]<<8,
    opc_tbl = init_opcode_table(),
    bus = bus,
  }
}