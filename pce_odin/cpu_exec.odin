package main

import "core:fmt"
import "base:intrinsics"
import "core:testing"

cpu_exec_instr :: proc(cpu: ^CPU) {
  cpu_check_irq(cpu)

  pc_start := cpu.pc

  cycle_start := cpu.bus.clocks

  opc := cpu_read_pc_u8(cpu)
  opc_info := cpu.opc_tbl[opc]
  adr := adr_decode(cpu, opc_info.adr)

  trace_instr(cpu, opc, pc_start, opc_info, adr)

  switch opc_info.instr {
  case .BRK:
    cpu_read_pc_u8(cpu)
    cpu_stk_push_u16(cpu, cpu.pc)
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
    adr_mode_write_u8(cpu, adr, 0)
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
    val := adr_mode_read_u8(cpu, adr)
    if cpu.p.dec {
    	res, car := bcd_add(val, cpu.a, cpu.p.car)
     
     	cpu.p.neg = (res&0x80) > 0
     	cpu.p.car = car
      // TODO: this is probably wrong
      cpu.p.ovf = (~(cpu.a ~ val) & (cpu.a ~ res) & 0x80) > 0
      cpu.p.zer = res == 0
      cpu.a = res
    } else {
      calc := cast(u16)cpu.a + cast(u16)val + cast(u16)cpu.p.car
      result := cast(u8)(calc&0xFF)

      cpu.p.neg = (result & 0x80) > 0
      cpu.p.car = calc > 0xFF
      // learned from mesen ^^ bitwise (sign(a) == sign(val) && sign(a) != sign(result))
      cpu.p.ovf = (~(cpu.a ~ val) & (cpu.a ~ result) & 0x80) > 0
      cpu.p.zer = result == 0

      cpu.a = result
    }
  case .SBC:
    val := adr_mode_read_u8(cpu, adr)
    if cpu.p.dec {
   		res, car := bcd_sub(val, cpu.a, cpu.p.car)
    
    	cpu.p.neg = (res&0x80) > 0
    	cpu.p.car = car
      // TODO: this is probably wrong
      cpu.p.ovf = (~(cpu.a ~ val) & (cpu.a ~ res) & 0x80) > 0
      cpu.p.zer = res == 0
      cpu.a = res
    } else {
	    calc := cast(u16)cpu.a + cast(u16)~val + cast(u16)cpu.p.car
	    result := cast(u8)(calc&0xFF)
	
	    cpu.p.neg = (result & 0x80) > 0
	    cpu.p.car = calc > 0xFF
	    cpu.p.ovf = (~(cpu.a ~ val) & (cpu.a ~ result) & 0x80) > 0
	    cpu.p.zer = result == 0
	
	    cpu.a = result
    }
  case .RTS:
    cpu_dummy_read(cpu)
    cpu.pc = cpu_stk_pop_u16(cpu)
    cpu.pc += 1
    trace_indent -= 1
  case .CMP:
    check := adr_mode_read_u8(cpu, adr)
    result := cpu.a-check
    cpu.p.car = cpu.a>=check
    cpu.p.zer = cpu.a==check
    cpu.p.neg = (result&0x80) > 0
  case .CPX:
    check := adr_mode_read_u8(cpu, adr)
    result := cpu.x-check
    cpu.p.car = cpu.x>=check
    cpu.p.zer = cpu.x==check
    cpu.p.neg = (result&0x80) > 0
  case .CPY:
    check := adr_mode_read_u8(cpu, adr)
    result := cpu.y-check
    cpu.p.car = cpu.y>=check
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
    if (val&(1<<opc_info.extra)) != 0 {
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
    cpu.p = cast(CPU_Flags)cpu_stk_pop_u8(cpu)
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
    res := (val << 1) | (cpu.p.car ? 1 : 0)
    cpu_set_nz(cpu, res)
    cpu.p.car = (val & 0x80) != 0
    adr_mode_write_u8(cpu, adr, res)
  case .ROR:
    val := adr_mode_read_u8(cpu, adr) 
    res := (val >> 1) | (cpu.p.car ? 0x80 : 0)
    cpu_set_nz(cpu, res)
    cpu.p.car = (val & 0x1) != 0
    adr_mode_write_u8(cpu, adr, res)
  case .ASL:
    val := adr_mode_read_u8(cpu, adr) 
    carry := val & 0x80
    res := val << 1
    cpu_set_nz(cpu, res)
    cpu.p.car = carry != 0
    adr_mode_write_u8(cpu, adr, res)
  case .LSR:
    val := adr_mode_read_u8(cpu, adr) 
    carry := val & 0x1
    res := val >> 1
    cpu_set_nz(cpu, res)
    cpu.p.car = carry != 0
    adr_mode_write_u8(cpu, adr, res)
  case .NOP:
    cpu_dummy_read(cpu)
  case .RTI:
    cpu.p = cast(CPU_Flags)cpu_stk_pop_u8(cpu)
    cpu.pc = cpu_stk_pop_u16(cpu)
  case .TST:
    imm := adr_mode_read_imm(cpu, adr)
    val := adr_mode_read_u8(cpu, adr)
    result := imm & val
    cpu.p.neg = (val & 0x80) != 0
    cpu.p.ovf = (val & 0x40) != 0
    cpu.p.zer = result == 0
  case .TRB:
    val := adr_mode_read_u8(cpu, adr)
    result := val & ~cpu.a
    cpu.p.neg = (val & 0x80) != 0
    cpu.p.ovf = (val & 0x40) != 0
    cpu.p.zer = result == 0
    adr_mode_write_u8(cpu, adr, result)
  case .TSB:
    val := adr_mode_read_u8(cpu, adr)
    result := cpu.a | val
    cpu.p.neg = (val & 0x80) != 0
    cpu.p.ovf = (val & 0x40) != 0
    cpu.p.zer = result == 0
    adr_mode_write_u8(cpu, adr, result)
  case .BIT:
    val := adr_mode_read_u8(cpu, adr)
    result := cpu.a & val
    cpu.p.neg = (val & 0x80) != 0
    cpu.p.ovf = (val & 0x40) != 0
    cpu.p.zer = result == 0
  case .UNK:
    trace_instr(cpu, opc, pc_start, opc_info, adr, stdout = true)
    unimplemented(fmt.aprintf("%02X", opc))
  case .SET:
    unimplemented("SET")
  case .SED:
	  cpu_dummy_read(cpu)
  	cpu.p.dec = true
  }

  // cycle_end := cpu.bus.clocks
  // mul : uint = cpu.fast ? 1 : 4

  // if opc_info.ref_cyc != 0 && (cycle_end-cycle_start) != opc_info.ref_cyc*mul {
  //   fmt.printf("CYCLE MISMATCH: %v (%02X) %v vs %v\n", opc_info.instr, opc, (cycle_end-cycle_start), opc_info.ref_cyc*mul)
  // }

  cpu.p.mem = false
}

// NOTE: only digit (low 4) bits are added, carry goes into bit 4
bcd_add_digit :: proc(a: u8, b: u8, carry: bool = false) -> u8 {
  result := a&0xF + b&0xF + cast(u8)carry&0x1
  if result >= 10 {
    result -= 10
    result |= 0x10
  }

  return result
}

// NOTE: only digit (low 4) bits are subtracted, carry goes into bit 4
bcd_sub_digit :: proc(a: u8, b: u8, carry: bool = true) -> u8 {
  result := a&0xF - b&0xF + (cast(u8)carry&0x1-1)
  if result & 0x80 > 0 {
  	result = (result-6)&0xF
    result |= 0x10
  }

  return result
}

bcd_add :: proc(a: u8, b: u8, carry: bool = false) -> (u8, bool) {
  lo := bcd_add_digit(a, b, carry)
  hi := bcd_add_digit(a>>4, b>>4, (lo & 0x10) > 0)

  return (lo&0xF)|(hi<<4), hi & 0x10 != 0
}

bcd_sub :: proc(a, b: u8, carry: bool = true) -> (u8, bool) {
  lo := bcd_sub_digit(a, b, carry)
  hi := bcd_sub_digit(a>>4, b>>4, (lo & 0x10) == 0)

  return (lo&0xF)|(hi<<4), hi & 0x10 == 0
}

@(test)
test_bcd_add :: proc(t: ^testing.T) {
	bcd_add_chk :: proc(t: ^testing.T, a: u8, b: u8, carry: bool, res: u8, car: bool) {
		val, carry := bcd_add(a, b, carry)
		testing.expectf(t, val == res, "expected %02X got %02X", res, val)
		testing.expectf(t, carry == car, "expected %v got %v", car, carry)
	}
	
	testing.expect(t, bcd_add_digit(0x1, 0x2) == 0x03)
  testing.expect(t, bcd_add_digit(0x9, 0x3) == 0x12)
  testing.expect(t, bcd_add_digit(0x9, 0x9) == 0x18)
  testing.expect(t, bcd_add_digit(0x9, 0x9, true) == 0x19)
  testing.expect(t, bcd_add_digit(0x5, 0x5, false) == 0x10)
  testing.expect(t, bcd_add_digit(0x5, 0x4, true) == 0x10)
  
  bcd_add_chk(t, 0x10, 0x20, false, 0x30, false)
  bcd_add_chk(t, 0x99, 0x99, false, 0x98, true)
  bcd_add_chk(t, 0x99, 0x99, true, 0x99, true)
  bcd_add_chk(t, 0x12, 0x67, false, 0x79, false)
  bcd_add_chk(t, 0x12, 0x69, false, 0x81, false)
  bcd_add_chk(t, 0x12, 0x69, true, 0x82, false)
  bcd_add_chk(t, 0x50, 0x50, false, 0x00, true)
  bcd_add_chk(t, 0x50, 0x50, true, 0x01, true)
  bcd_add_chk(t, 0x05, 0x05, false, 0x10, false)
  bcd_add_chk(t, 0x11, 0x11, false, 0x22, false)
}

@(test)
test_bcd_sub :: proc(t: ^testing.T) {
	bcd_sub_chk :: proc(t: ^testing.T, a: u8, b: u8, carry: bool, res: u8, car: bool, loc := #caller_location) {
		val, carry := bcd_sub(a, b, carry)
		testing.expectf(t, val == res, "expected %02X got %02X", res, val, loc=loc)
		testing.expectf(t, carry == car, "expected %v got %v", car, carry, loc=loc)
	}
	
	testing.expect(t, bcd_sub_digit(0x0, 0x1, true) == 0x19)
	testing.expect(t, bcd_sub_digit(0x9, 0x4, true) == 0x5)
	testing.expect(t, bcd_sub_digit(0x9, 0x4, false) == 0x4)
	testing.expect(t, bcd_sub_digit(0x1, 0x1, false) == 0x19)
	testing.expect(t, bcd_sub_digit(0x0, 0x0, false) == 0x19)
	testing.expect(t, bcd_sub_digit(0x9, 0x9, true) == 0x00)
	testing.expect(t, bcd_sub_digit(0x1, 0x4, true) == 0x17)
	testing.expect(t, bcd_sub_digit(0x0, 0x9, true) == 0x11)
	
	bcd_sub_chk(t, 0x01, 0x00, true, 0x01, true)
	bcd_sub_chk(t, 0x10, 0x05, true, 0x05, true)
	bcd_sub_chk(t, 0x00, 0x01, true, 0x99, false)
	bcd_sub_chk(t, 0x00, 0x02, true, 0x98, false)
	bcd_sub_chk(t, 0x00, 0x09, true, 0x91, false)
	bcd_sub_chk(t, 0x00, 0x29, true, 0x71, false)
	bcd_sub_chk(t, 0x00, 0x19, true, 0x81, false)
	bcd_sub_chk(t, 0x00, 0x01, false, 0x98, false)
	bcd_sub_chk(t, 0x00, 0x02, false, 0x97, false)
	bcd_sub_chk(t, 0x00, 0x09, false, 0x90, false)
	bcd_sub_chk(t, 0x00, 0x29, false, 0x70, false)
	bcd_sub_chk(t, 0x00, 0x19, false, 0x80, false)
	bcd_sub_chk(t, 0x99, 0x19, true, 0x80, true)
	bcd_sub_chk(t, 0x99, 0x19, false, 0x79, true)
}
