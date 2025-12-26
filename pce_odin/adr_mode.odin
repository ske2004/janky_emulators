package main

import "core:fmt"

// (n)    = indirect
//  n , o = offsetd
//  n + m = no idea (immediate + thing)
Adr_Mode :: enum {
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

Adr_Acc :: struct {}

Adr_Zpg_Indirect :: struct {
  addr: u8,
  inner_offs: u8,
  outer_offs: u16,
}

Adr_Basic_Indirect :: struct {
  addr: u16,
  inner_offs: u16,
  outer_offs: u16,
}

Adr_Zpg :: struct {
  addr: u8,
  offs: u8,
}

Adr_Basic :: struct {
  addr: u16,
  offs: u16,
}

Adr_Imm :: struct {
  val: u8,
} 

Adr_Rel :: struct {
  val: i8,
}

Adr_Basic_Rel :: struct {
  addr: u16,
  rel: i8,
}

Adr_Imm_Zpg :: struct {
  imm: u8,
  addr: u8,
  offs: u8,
}

Adr_Imm_Basic :: struct {
  imm: u8,
  addr: u16,
  offs: u16,
}

Adr_Decoded :: union {
  Adr_Acc,
  Adr_Zpg,
  Adr_Basic,
  Adr_Zpg_Indirect,
  Adr_Basic_Indirect,
  Adr_Imm,
  Adr_Rel,
  Adr_Basic_Rel,
  Adr_Imm_Zpg,
  Adr_Imm_Basic,
}

adr_decode :: proc(cpu: ^CPU, adr: Adr_Mode) -> Adr_Decoded {
  #partial switch adr {
  case .Imp: return nil
  case .Imm: return Adr_Imm{val = cpu_read_pc_u8(cpu)}
  case .Acc: return Adr_Acc{}
  case .Abs: return Adr_Basic{addr = cpu_read_pc_u16(cpu)}
  case .Abx: return Adr_Basic{addr = cpu_read_pc_u16(cpu), offs = cast(u16)cpu.x}
  case .Aby: return Adr_Basic{addr = cpu_read_pc_u16(cpu), offs = cast(u16)cpu.y}
  case .Zpg: return Adr_Zpg{addr = cpu_read_pc_u8(cpu)}
  case .Zpx: return Adr_Zpg{addr = cpu_read_pc_u8(cpu), offs = cpu.x}
  case .Zpy: return Adr_Zpg{addr = cpu_read_pc_u8(cpu), offs = cpu.y}
  case .Zpi: return Adr_Zpg_Indirect{addr = cpu_read_pc_u8(cpu)}
  case .Zxi: return Adr_Zpg_Indirect{addr = cpu_read_pc_u8(cpu), inner_offs = cpu.x}
  case .Ziy: return Adr_Zpg_Indirect{addr = cpu_read_pc_u8(cpu), outer_offs = cast(u16)cpu.y}
  case .Axi: return Adr_Basic_Indirect{addr = cpu_read_pc_u16(cpu), inner_offs = cast(u16)cpu.x}
  case .Abi: return Adr_Basic_Indirect{addr = cpu_read_pc_u16(cpu)}
  case .Rel: return Adr_Rel{val = transmute(i8)cpu_read_pc_u8(cpu)}
  case .Izp:
    imm := cpu_read_pc_u8(cpu)
    return Adr_Imm_Zpg{ imm = imm, addr = cpu_read_pc_u8(cpu) }
  case .Izx:
    imm := cpu_read_pc_u8(cpu)
    return Adr_Imm_Zpg{ imm = imm, addr = cpu_read_pc_u8(cpu), offs = cpu.x }
  case .Iab:
    imm := cpu_read_pc_u8(cpu)
    return Adr_Imm_Basic{ imm = imm, addr = cpu_read_pc_u16(cpu) }
  case .Iax:
    imm := cpu_read_pc_u8(cpu)
    return Adr_Imm_Basic{ imm = imm, addr = cpu_read_pc_u16(cpu), offs = cast(u16)cpu.x }
  case .Zpr:
    zpg := cast(u16)cpu_read_pc_u8(cpu)
    rel := cast(i8)cpu_read_pc_u8(cpu)
    return Adr_Basic_Rel{addr = zpg|ZPG_START, rel = rel}
  case: unimplemented(fmt.aprintf("ADDR MODE %v", adr))
  }
}

adr_mode_read_u8 :: proc(cpu: ^CPU, adr: Adr_Decoded) -> u8 {
  switch v in adr {
  case Adr_Acc:         return cpu.a
  case Adr_Basic:       return cpu_read_u8(cpu, v.addr+v.offs)
  case Adr_Zpg:         return cpu_read_u8(cpu, cast(u16)(v.addr+v.offs)|ZPG_START)
  case Adr_Zpg_Indirect:
    zpg_addr := cpu_read_u16_zpg(cpu, v.addr+v.inner_offs)+v.outer_offs
    return cpu_read_u8(cpu, zpg_addr)
  case Adr_Imm:            return v.val
  case Adr_Rel:            panic("use adr_mode_read_rel")
  case Adr_Basic_Rel:      return cpu_read_u8(cpu, v.addr)
  case Adr_Imm_Basic:      return cpu_read_u8(cpu, v.addr+v.offs)
  case Adr_Imm_Zpg:        return cpu_read_u8(cpu, cast(u16)(v.addr+v.offs)|ZPG_START)
  case Adr_Basic_Indirect: 
    abs_addr := cpu_read_u16(cpu, v.addr+v.inner_offs)+v.outer_offs
    return cpu_read_u8(cpu, abs_addr)
  case:                  unimplemented("read on imp")
  }
}

adr_mode_read_imm :: proc(cpu: ^CPU, adr: Adr_Decoded) -> u8 {
  #partial switch v in adr {
  case Adr_Imm_Basic: return v.imm
  case Adr_Imm_Zpg:   return v.imm
  case:               unimplemented("read_imm only for tst")
  }
}

adr_mode_read_rel :: proc(cpu: ^CPU, adr: Adr_Decoded) -> i8 {
  #partial switch v in adr {
  case Adr_Rel:       return v.val
  case Adr_Basic_Rel: return v.rel
  case:               unimplemented("not rel")
  }
}

adr_mode_read_addr :: proc(cpu: ^CPU, adr: Adr_Decoded) -> u16 {
  #partial switch v in adr {
  case Adr_Basic:          return v.addr+v.offs
  case Adr_Basic_Indirect: return cpu_read_u16(cpu, v.addr+v.inner_offs)+v.outer_offs
  case:                    unimplemented("invalid addr")
  }
}

adr_mode_write_u8 :: proc(cpu: ^CPU, adr: Adr_Decoded, val: u8) {
  switch v in adr {
  case Adr_Acc:          cpu.a = val
  case Adr_Zpg:          cpu_write_u8(cpu, cast(u16)(v.addr+v.offs)|ZPG_START, val)
  case Adr_Basic:        cpu_write_u8(cpu, v.addr+v.offs, val)
  case Adr_Basic_Rel:    unimplemented("write on basic rel")
  case Adr_Zpg_Indirect: 
    zpg_addr := cpu_read_u16_zpg(cpu, v.addr+v.inner_offs)+v.outer_offs
    cpu_write_u8(cpu, zpg_addr, val)
  case Adr_Imm:         unimplemented("write on imm")
  case Adr_Rel:         unimplemented("write on rel")
  case Adr_Imm_Basic:   unimplemented("write on imm_basic")
  case Adr_Imm_Zpg:     unimplemented("write on imm_zpg")
  case Adr_Basic_Indirect: 
    abs_addr := cpu_read_u16(cpu, v.addr+v.inner_offs)+v.outer_offs
    cpu_write_u8(cpu, abs_addr, val)
  case:                unimplemented("write on imp")
  }
}

