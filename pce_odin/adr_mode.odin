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

AdrZpgIndirect :: struct {
  addr: u8,
  inner_offs: u8,
  outer_offs: u16,
}

AdrBasicIndirect :: struct {
  addr: u16,
  inner_offs: u16,
  outer_offs: u16,
}

AdrZpg :: struct {
  addr: u8,
  offs: u8,
}

AdrBasic :: struct {
  addr: u16,
  offs: u16,
}

AdrImm :: struct {
  val: u8,
} 

AdrRel :: struct {
  val: i8,
}

AdrBasicRel :: struct {
  addr: u16,
  rel: i8,
}

AdrImmZpg :: struct {
  imm: u8,
  addr: u8,
  offs: u8,
}

AdrImmBasic :: struct {
  imm: u8,
  addr: u16,
  offs: u16,
}

AdrDecoded :: union {
  AdrAcc,
  AdrZpg,
  AdrBasic,
  AdrZpgIndirect,
  AdrBasicIndirect,
  AdrImm,
  AdrRel,
  AdrBasicRel,
  AdrImmZpg,
  AdrImmBasic,
}

adr_decode :: proc(cpu: ^Cpu, adr: AdrMode) -> AdrDecoded {
  #partial switch adr {
  case .Imp: return nil
  case .Imm: return AdrImm{val = cpu_read_pc_u8(cpu)}
  case .Acc: return AdrAcc{}
  case .Abs: return AdrBasic{addr = cpu_read_pc_u16(cpu)}
  case .Abx: return AdrBasic{addr = cpu_read_pc_u16(cpu), offs = cast(u16)cpu.x}
  case .Aby: return AdrBasic{addr = cpu_read_pc_u16(cpu), offs = cast(u16)cpu.y}
  case .Zpg: return AdrZpg{addr = cpu_read_pc_u8(cpu) }
  case .Zpx: return AdrZpg{addr = cpu_read_pc_u8(cpu), offs = cpu.x}
  case .Zpy: return AdrZpg{addr = cpu_read_pc_u8(cpu), offs = cpu.y}
  case .Zpi: return AdrZpgIndirect{addr = cpu_read_pc_u8(cpu) }
  case .Zxi: return AdrZpgIndirect{addr = cpu_read_pc_u8(cpu), inner_offs = cpu.x }
  case .Ziy: return AdrZpgIndirect{addr = cpu_read_pc_u8(cpu), outer_offs = cast(u16)cpu.y }
  case .Axi: return AdrBasicIndirect{ addr = cpu_read_pc_u16(cpu), inner_offs = cast(u16)cpu.x }
  case .Abi: return AdrBasicIndirect{ addr = cpu_read_pc_u16(cpu) }
  case .Rel: return AdrRel{val = transmute(i8)cpu_read_pc_u8(cpu)}
  case .Izp:
    imm := cpu_read_pc_u8(cpu)
    return AdrImmZpg{ imm = imm, addr = cpu_read_pc_u8(cpu) }
  case .Izx:
    imm := cpu_read_pc_u8(cpu)
    return AdrImmZpg{ imm = imm, addr = cpu_read_pc_u8(cpu), offs = cpu.x }
  case .Iab:
    imm := cpu_read_pc_u8(cpu)
    return AdrImmBasic{ imm = imm, addr = cpu_read_pc_u16(cpu) }
  case .Iax:
    imm := cpu_read_pc_u8(cpu)
    return AdrImmBasic{ imm = imm, addr = cpu_read_pc_u16(cpu), offs = cast(u16)cpu.x }
  case .Zpr:
    zpg := cast(u16)cpu_read_pc_u8(cpu)
    rel := cast(i8)cpu_read_pc_u8(cpu)
    return AdrBasicRel{addr = zpg|ZPG_START, rel = rel}
  case: unimplemented(fmt.aprintf("ADDR MODE %v", adr))
  }
}

adr_mode_read_u8 :: proc(cpu: ^Cpu, adr: AdrDecoded) -> u8 {
  switch v in adr {
  case AdrAcc:         return cpu.a
  case AdrBasic:       return cpu_read_u8(cpu, v.addr+v.offs)
  case AdrZpg:         return cpu_read_u8(cpu, cast(u16)(v.addr+v.offs)|ZPG_START)
  case AdrZpgIndirect:
    zpg_addr := cpu_read_u16_zpg(cpu, v.addr+v.inner_offs)+v.outer_offs
    return cpu_read_u8(cpu, zpg_addr)
  case AdrImm:           return v.val
  case AdrRel:           panic("use adr_mode_read_rel")
  case AdrBasicRel:      return cpu_read_u8(cpu, v.addr)
  case AdrImmBasic:      return cpu_read_u8(cpu, v.addr+v.offs)
  case AdrImmZpg:        return cpu_read_u8(cpu, cast(u16)(v.addr+v.offs)|ZPG_START)
  case AdrBasicIndirect: 
    abs_addr := cpu_read_u16(cpu, v.addr+v.inner_offs)+v.outer_offs
    return cpu_read_u8(cpu, abs_addr)
  case:                  unimplemented("read on imp")
  }
}

adr_mode_read_imm :: proc(cpu: ^Cpu, adr: AdrDecoded) -> u8 {
  #partial switch v in adr {
  case AdrImmBasic: return v.imm
  case AdrImmZpg:   return v.imm
  case:             unimplemented("read_imm only for tst")
  }
}

adr_mode_read_rel :: proc(cpu: ^Cpu, adr: AdrDecoded) -> i8 {
  #partial switch v in adr {
  case AdrRel:      return v.val
  case AdrBasicRel: return v.rel
  case:             unimplemented("not rel")
  }
}

adr_mode_read_addr :: proc(cpu: ^Cpu, adr: AdrDecoded) -> u16 {
  #partial switch v in adr {
  case AdrBasic:         return v.addr+v.offs
  case AdrBasicIndirect: return cpu_read_u16(cpu, v.addr+v.inner_offs)+v.outer_offs
  case:                  unimplemented("invalid addr")
  }
}

adr_mode_write_u8 :: proc(cpu: ^Cpu, adr: AdrDecoded, val: u8) {
  switch v in adr {
  case AdrAcc:         cpu.a = val
  case AdrZpg:         cpu_write_u8(cpu, cast(u16)(v.addr+v.offs)|ZPG_START, val)
  case AdrBasic:       cpu_write_u8(cpu, v.addr+v.offs, val)
  case AdrBasicRel:    unimplemented("write on basic rel")
  case AdrZpgIndirect: 
    zpg_addr := cpu_read_u16_zpg(cpu, v.addr+v.inner_offs)+v.outer_offs
    cpu_write_u8(cpu, zpg_addr, val)
  case AdrImm:         unimplemented("write on imm")
  case AdrRel:         unimplemented("write on rel")
  case AdrImmBasic:    unimplemented("write on imm_basic")
  case AdrImmZpg:      unimplemented("write on imm_zpg")
  case AdrBasicIndirect: 
    abs_addr := cpu_read_u16(cpu, v.addr+v.inner_offs)+v.outer_offs
    cpu_write_u8(cpu, abs_addr, val)
  case:                unimplemented("write on imp")
  }
}

