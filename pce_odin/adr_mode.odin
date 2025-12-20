package main

import "core:fmt"

// (n)    = indirect
//  n , o = offsetd
//  n + m = no idea (immediate + thing)
AdrMode :: enum {
  Imp, // implied
  Imm, // immediate
  Zpg, // zeropage
  Zpx, // zeropage, x
  Zpy, // zeropage, y
  Zpr, // zeropage, rel
  Zpi, // (zeropage)
  Zxi, // (zeropage, x)
  Ziy, // (zeropage), y
  Abs, // absolute
  Abx, // absolute, x
  Aby, // absolute, y
  Abi, // (absolute)
  Axi, // (absoulte, x)
  Rel, // relative
  Izp, // immediate + zeropage
  Izx, // immediate + zeropage, x
  Iab, // immediate + absolute
  Iax, // immediate + absolute, x
  Acc, // A
}

AdrAcc :: struct {}

AdrIndirect :: struct {
  addr: u16,
  inner_offs: u16,
  outer_offs: u16
}

AdrBasic :: struct {
  addr: u16,
  offs: u16,
}

AdrImm :: struct {
  val: u8
} 

AdrRel :: struct {
  val: i8
}

AdrDecoded :: union {
  AdrAcc,
  AdrBasic,
  AdrIndirect,
  AdrImm,
  AdrRel,
}

adr_decode :: proc(cpu: ^Cpu, adr: AdrMode) -> AdrDecoded {
  #partial switch adr {
    case .Imp: return nil
    case .Imm: return AdrImm{val = cpu_read_pc_u8(cpu)}
    case .Acc: return AdrAcc{}
    case .Abs: return AdrBasic{addr = cpu_read_pc_u16(cpu)}
    case .Abx: return AdrBasic{addr = cpu_read_pc_u16(cpu), offs = cast(u16)cpu.x}
    case .Aby: return AdrBasic{addr = cpu_read_pc_u16(cpu), offs = cast(u16)cpu.y}
    case .Zpg: return AdrBasic{addr = cast(u16)cpu_read_pc_u8(cpu)|ZPG_START }
    case .Zxi: return AdrIndirect{addr = cast(u16)cpu_read_pc_u8(cpu)|ZPG_START, inner_offs = cast(u16)cpu.x }
    case .Ziy: return AdrIndirect{addr = cast(u16)cpu_read_pc_u8(cpu)|ZPG_START, outer_offs = cast(u16)cpu.y }
    case .Rel: return AdrRel{val = transmute(i8)cpu_read_pc_u8(cpu)}
    case: unimplemented(fmt.aprintf("ADDR MODE %v", adr))
  }
}

adr_mode_read_u8 :: proc(cpu: ^Cpu, adr: AdrDecoded) -> u8 {
  switch v in adr {
    case AdrAcc:      return cpu.a
    case AdrBasic:    return cpu_read_u8(cpu, v.addr+v.offs)
    case AdrIndirect: return cpu_read_u8(cpu, cpu_read_u16(cpu, v.addr+v.inner_offs)+v.outer_offs)
    case AdrImm:      return v.val
    case AdrRel:      panic("use adr_mode_read_rel")
    case:             unimplemented("read on imp")
  }
}

adr_mode_read_rel :: proc(cpu: ^Cpu, adr: AdrDecoded) -> i8 {
  #partial switch v in adr {
    case AdrRel: return v.val
    case:        unimplemented("not rel")
  }
}

adr_mode_read_addr :: proc(cpu: ^Cpu, adr: AdrDecoded) -> u16 {
  #partial switch v in adr {
    case AdrBasic: return v.addr
    case:          unimplemented("invalid addr")
  }
}

adr_mode_write_u8 :: proc(cpu: ^Cpu, adr: AdrDecoded, val: u8) {
  switch v in adr {
    case AdrAcc:      cpu.a = val
    case AdrBasic:    cpu_write_u8(cpu, v.addr+v.offs, val)
    case AdrIndirect: cpu_write_u8(cpu, cpu_read_u16(cpu, v.addr+v.inner_offs)+v.outer_offs, val)
    case AdrImm:      unimplemented("write on imm")
    case AdrRel:      unimplemented("write on rel")
    case:             unimplemented("write on imp")
  }
}

