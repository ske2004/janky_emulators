package main

import "core:fmt"
import "base:intrinsics"

cpu_exec_instr :: proc(cpu: ^Cpu) {
  cpu_check_irq(cpu)

  if cpu.pc == 0xE17E {
    is_tracing_enabled = true
  }

  pc_start := cpu.pc

  opc := cpu_read_pc_u8(cpu)
  opc_info := cpu.opc_tbl[opc]
  adr := adr_decode(cpu, opc_info.adr)

  trace_instr(cpu, opc, pc_start, opc_info, adr)

  switch opc_info.instr {
  case .BRK:
    cpu_read_pc_u8(cpu)
    cpu_stk_push_u16(cpu, cpu.pc-1)
    cpu_stk_push_u8(cpu, cast(u8)cpu.p)
    cpu.pc = cpu_read_u16(cpu, IRQ2_START)
    cpu.p.int = true
    cpu.p.brk = true
    cpu.p.dec = false
  case .CSH:
    cpu_dummy_read(cpu)
    cpu.fast = true
  case .CSL:
    cpu_dummy_read(cpu)
    cpu.fast = false
  case .CLD:
    cpu_dummy_read(cpu)
    cpu.p.dec = false
  case .CLA:
    cpu_dummy_read(cpu)
    cpu.a = 0
  case .CLX:
    cpu_dummy_read(cpu)
    cpu.x = 0
  case .CLY:
    cpu_dummy_read(cpu)
    cpu.y = 0
  case .LDA:
    cpu.a = adr_mode_read_u8(cpu, adr)
    cpu_set_nz(cpu, cpu.a)
  case .LDX:
    cpu.x = adr_mode_read_u8(cpu, adr)
    cpu_set_nz(cpu, cpu.x)
  case .LDY:
    cpu.y = adr_mode_read_u8(cpu, adr)
    cpu_set_nz(cpu, cpu.y)
  case .TAY:
    cpu_dummy_read(cpu)
    cpu.y = cpu.a
    cpu_set_nz(cpu, cpu.y)
  case .TYA:
    cpu_dummy_read(cpu)
    cpu.a = cpu.y
    cpu_set_nz(cpu, cpu.a)
  case .TAX:
    cpu_dummy_read(cpu)
    cpu.x = cpu.a
    cpu_set_nz(cpu, cpu.x)
  case .TXA:
    cpu_dummy_read(cpu)
    cpu.a = cpu.x
    cpu_set_nz(cpu, cpu.a)
  case .TSX:
    cpu_dummy_read(cpu)
    cpu.x = cpu.sp
    cpu_set_nz(cpu, cpu.x)
  case .TXS:
    cpu_dummy_read(cpu)
    cpu.sp = cpu.x
  case .STZ:
    adr_mode_write_u8(cpu, adr, 0);
  case .TAM:
    mask := cpu_read_pc_u8(cpu)
    log_instr_info("TAM: %02X=%02X", mask, cpu.a)
    for i in 0..<8 {
      if (mask&(1<<u8(i))) > 0 {
        log_instr_info("TAM: Mapping %04X to page %06X (page: %02X)", i*0x2000, cast(u32)cpu.a<<BANK_SHIFT, cpu.a)
        cpu.mpr[i] = cpu.a
      }
    }
  case .TDD:
    block_transfer(cpu, srcop=.DEC, dstop=.DEC)
  case .TIA:
    block_transfer(cpu, srcop=.INC, dstop=.INCDEC)
  case .TIN:
    block_transfer(cpu, srcop=.INC, dstop=.NONE)
  case .TII:
    block_transfer(cpu, srcop=.INC, dstop=.INC)
  case .TAI:
    block_transfer(cpu, srcop=.INCDEC, dstop=.INC)
  case .JMP:
    dest := adr_mode_read_addr(cpu, adr)
    cpu.pc = dest
  case .JSR:
    dest := adr_mode_read_addr(cpu, adr)
    cpu_stk_push_u16(cpu, cpu.pc-1)
    cpu.pc = dest
    trace_indent += 1
  case .BSR:
    dest := adr_mode_read_rel(cpu, adr)
    cpu_stk_push_u16(cpu, cpu.pc-1)
    cpu.pc += cast(u16)i16(dest)
    trace_indent += 1
  case .STA:
    adr_mode_write_u8(cpu, adr, cpu.a)
  case .STX:
    adr_mode_write_u8(cpu, adr, cpu.x)
  case .STY:
    adr_mode_write_u8(cpu, adr, cpu.y)
  case .ADC:
    // i dont know any better
    val := adr_mode_read_u8(cpu, adr)
    calc := cast(u16)cpu.a + cast(u16)val + cast(u16)cpu.p.car
    ovf := (cpu.a&0x7F) + (val&0x7F) + cast(u8)cpu.p.car
    result := cast(i8)(calc&0xFF)

    cpu.p.neg = result < 0
    cpu.p.car = (calc & 0x0100) > 0
    cpu.p.ovf = ((ovf & 0x80) > 0) ~ cpu.p.car
    cpu.p.zer = result == 0

    cpu.a = cast(u8)result
  case .SBC:
    // i dont know any better
    val := adr_mode_read_u8(cpu, adr)
    calc := cast(u16)cpu.a - cast(u16)val - cast(u16)(!cpu.p.car)
    ovf := (cpu.a&0x7F) - (val&0x7F) - cast(u8)(!cpu.p.car)
    result := cast(i8)(calc&0xFF)

    cpu.p.neg = result < 0
    cpu.p.car = (calc & 0xFF00) == 0
    cpu.p.ovf = ((ovf & 0x80) > 0) ~ cpu.p.car
    cpu.p.zer = result == 0

    cpu.a = cast(u8)result
  case .RTS:
    cpu_dummy_read(cpu)
    cpu.pc = cpu_stk_pop_u16(cpu)
    cpu.pc += 1
    trace_indent -= 1
  case .CMP:
    check := adr_mode_read_u8(cpu, adr)
    result := cpu.a-check
    cpu.p.car = ((cast(u16)cpu.a-cast(u16)check)&0xFF00) == 0
    cpu.p.zer = cpu.a==check
    cpu.p.neg = (result&0x80) > 0
  case .CPX:
    check := adr_mode_read_u8(cpu, adr)
    result := cpu.x-check
    cpu.p.car = ((cast(u16)cpu.x-cast(u16)check)&0xFF00) == 0
    cpu.p.zer = cpu.x==check
    cpu.p.neg = (result&0x80) > 0
  case .CPY:
    check := adr_mode_read_u8(cpu, adr)
    result := cpu.y-check
    cpu.p.car = ((cast(u16)cpu.y-cast(u16)check)&0xFF00) == 0
    cpu.p.zer = cpu.y==check
    cpu.p.neg = (result&0x80) > 0
  case .BEQ:
    if cpu.p.zer do cpu.pc += cast(u16)i16(adr_mode_read_rel(cpu, adr))
  case .BNE:
    if !cpu.p.zer do cpu.pc += cast(u16)i16(adr_mode_read_rel(cpu, adr))
  case .BMI:
    if cpu.p.neg do cpu.pc += cast(u16)i16(adr_mode_read_rel(cpu, adr))
  case .BPL:
    if !cpu.p.neg do cpu.pc += cast(u16)i16(adr_mode_read_rel(cpu, adr))
  case .BCS:
    if cpu.p.car do cpu.pc += cast(u16)i16(adr_mode_read_rel(cpu, adr))
  case .BCC:
    if !cpu.p.car do cpu.pc += cast(u16)i16(adr_mode_read_rel(cpu, adr))
  case .BVS:
    if cpu.p.ovf do cpu.pc += cast(u16)i16(adr_mode_read_rel(cpu, adr))
  case .BVC:
    if !cpu.p.ovf do cpu.pc += cast(u16)i16(adr_mode_read_rel(cpu, adr))
  case .BRA:
    cpu.pc += cast(u16)i16(adr_mode_read_rel(cpu, adr))
  case .CLI:
    cpu_dummy_read(cpu)
    cpu.p.int = false
  case .SEI:
    cpu_dummy_read(cpu)
    cpu.p.int = true
  case .CLC:
    cpu_dummy_read(cpu)
    cpu.p.car = false
  case .SEC:
    cpu_dummy_read(cpu)
    cpu.p.car = true
  case .CLV:
    cpu_dummy_read(cpu)
    cpu.p.ovf = false
  case .DEC:
    v := adr_mode_read_u8(cpu, adr)
    v -= 1
    cpu_set_nz(cpu, v)
    adr_mode_write_u8(cpu, adr, v)
  case .INC:
    v := adr_mode_read_u8(cpu, adr)
    v += 1
    cpu_set_nz(cpu, v)
    adr_mode_write_u8(cpu, adr, v)
  case .DEX:
    cpu_dummy_read(cpu)
    cpu.x -= 1
    cpu_set_nz(cpu, cpu.x)
  case .INX:
    cpu_dummy_read(cpu)
    cpu.x += 1
    cpu_set_nz(cpu, cpu.x)
  case .DEY:
    cpu_dummy_read(cpu)
    cpu.y -= 1
    cpu_set_nz(cpu, cpu.y)
  case .INY:
    cpu_dummy_read(cpu)
    cpu.y += 1
    cpu_set_nz(cpu, cpu.y)
  case .TMA:
    mask := cpu_read_pc_u8(cpu)
    if intrinsics.count_ones(mask) != 1 {
      panic("invalid popcount")
    }

    bit := intrinsics.count_trailing_zeros(mask)
    log_instr_info("TMA bit %02X %d = %02X", mask, bit, cpu.mpr[bit])
    cpu.a = cpu.mpr[bit]
  case .ST0:
    val := adr_mode_read_u8(cpu, adr)
    bus_write_u8(cpu.bus, 0x1FE000, val) // vdc ctrl reg
  case .ST1:
    val := adr_mode_read_u8(cpu, adr)
    bus_write_u8(cpu.bus, 0x1FE002, val) // vdc low reg
  case .ST2:
    val := adr_mode_read_u8(cpu, adr)
    bus_write_u8(cpu.bus, 0x1FE003, val) // vdc high reg
  case .RMB:
    val := adr_mode_read_u8(cpu, adr)
    val &= ~(1<<opc_info.extra)
    adr_mode_write_u8(cpu, adr, val)
  case .SMB:
    val := adr_mode_read_u8(cpu, adr)
    val |= 1<<opc_info.extra
    adr_mode_write_u8(cpu, adr, val)
  case .BBR:
    val := adr_mode_read_u8(cpu, adr)
    if (val&(1<<opc_info.extra)) == 0 {
      cpu.pc += cast(u16)i16(adr_mode_read_rel(cpu, adr))
    }
  case .BBS:
    val := adr_mode_read_u8(cpu, adr)
    if (val&(1<<opc_info.extra)) > 0 {
      cpu.pc += cast(u16)i16(adr_mode_read_rel(cpu, adr))
    }
  case .AND:
    val := adr_mode_read_u8(cpu, adr)
    cpu.a &= val
    cpu_set_nz(cpu, cpu.a)
  case .EOR:
    val := adr_mode_read_u8(cpu, adr)
    cpu.a ~= val
    cpu_set_nz(cpu, cpu.a)
  case .ORA:
    val := adr_mode_read_u8(cpu, adr)
    cpu.a |= val
    cpu_set_nz(cpu, cpu.a)
  case .PHP:
    cpu_dummy_read(cpu)
    cpu_stk_push_u8(cpu, cast(u8)cpu.p)
  case .PLP:
    cpu_dummy_read(cpu)
    cpu.p = cast(CpuFlags)cpu_stk_pop_u8(cpu)
  case .PHA:
    cpu_dummy_read(cpu)
    cpu_stk_push_u8(cpu, cpu.a)
  case .PHX:
    cpu_dummy_read(cpu)
    cpu_stk_push_u8(cpu, cpu.x)
  case .PHY:
    cpu_dummy_read(cpu)
    cpu_stk_push_u8(cpu, cpu.y)
  case .PLA:
    cpu_dummy_read(cpu)
    cpu.a = cpu_stk_pop_u8(cpu)
    cpu_set_nz(cpu, cpu.a)
  case .PLX:
    cpu_dummy_read(cpu)
    cpu.x = cpu_stk_pop_u8(cpu)
    cpu_set_nz(cpu, cpu.x)
  case .PLY:
    cpu_dummy_read(cpu)
    cpu.y = cpu_stk_pop_u8(cpu)
    cpu_set_nz(cpu, cpu.y)
  case .SAX:
    cpu_dummy_read(cpu)
    cpu.a, cpu.x = cpu.x, cpu.a
  case .SAY:
    cpu_dummy_read(cpu)
    cpu.a, cpu.y = cpu.y, cpu.a
  case .SXY:
    cpu_dummy_read(cpu)
    cpu.x, cpu.y = cpu.y, cpu.x
  case .ROL:
    val := adr_mode_read_u8(cpu, adr) 
    res := (val << 1) | u8(cpu.p.car)
    cpu_set_nz(cpu, res)
    cpu.p.car = (val & 0x80) > 0
    adr_mode_write_u8(cpu, adr, res)
  case .ROR:
    val := adr_mode_read_u8(cpu, adr) 
    res := (val >> 1) | (u8(cpu.p.car) << 7)
    cpu_set_nz(cpu, res)
    cpu.p.car = (val & 0x1) > 0
    adr_mode_write_u8(cpu, adr, res)
  case .ASL:
    val := adr_mode_read_u8(cpu, adr) 
    carry := val & 0x80
    res := val << 1
    cpu_set_nz(cpu, res)
    cpu.p.car = carry > 0
    adr_mode_write_u8(cpu, adr, res)
  case .LSR:
    val := adr_mode_read_u8(cpu, adr) 
    carry := val & 0x1
    res := val >> 1
    cpu_set_nz(cpu, res)
    cpu.p.car = carry > 0
    adr_mode_write_u8(cpu, adr, res)
  case .NOP:
    cpu_dummy_read(cpu)
  case .RTI:
    cpu.p = cast(CpuFlags)cpu_stk_pop_u8(cpu)
    cpu.pc = cpu_stk_pop_u16(cpu)
  case .TST:
    imm := adr_mode_read_imm(cpu, adr)
    val := adr_mode_read_u8(cpu, adr)
    result := imm & val
    cpu.p.neg = (val & 0x80) > 0
    cpu.p.ovf = (val & 0x40) > 0
    cpu.p.zer = result == 0
  case .TRB:
    val := adr_mode_read_u8(cpu, adr)
    result := cpu.a & ~val
    cpu.p.neg = (result & 0x80) > 0
    cpu.p.ovf = (result & 0x40) > 0
    cpu.p.zer = result == 0
    adr_mode_write_u8(cpu, adr, result)
  case .TSB:
    val := adr_mode_read_u8(cpu, adr)
    result := cpu.a | val
    cpu.p.neg = (val & 0x80) > 0
    cpu.p.ovf = (val & 0x40) > 0
    cpu.p.zer = result == 0
    adr_mode_write_u8(cpu, adr, result)
  case .BIT:
    val := adr_mode_read_u8(cpu, adr)
    result := cpu.a & val
    cpu.p.neg = (val & 0x80) > 0
    cpu.p.ovf = (val & 0x40) > 0
    cpu.p.zer = result == 0
  case .UNK:
    trace_instr(cpu, opc, pc_start, opc_info, adr, stdout = true)
    unimplemented(fmt.aprintf("%02X", opc))
  }

  cpu.p.mem = false
}