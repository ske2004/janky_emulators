// instruction tabels n stuff

package main

Instr :: enum {
  UNK,

  ADC,
  ASL,
  BBR, // + extra
  BBS, // + extra
  BCS,
  BEQ,
  BNE,
  BPL,
  BRA,
  BSR,
  CLA,
  CLC,
  CLD,
  CLI,
  CLX,
  CLY,
  CMP,
  CPY,
  CSH,
  CSL,
  DEC,
  DEX,
  DEY,
  INC,
  INX,
  INY,
  JMP,
  JSR,
  LDA,
  LDX,
  ORA,
  PHA,
  PHX,
  PHY,
  PLA,
  PLX,
  PLY,
  RMB, // + extra
  RTI,
  RTS,
  SBC,
  SEC,
  SEI,
  SMB, // + extra
  ST0,
  ST1,
  ST2,
  STA,
  STX,
  STZ,
  TAI,
  TAM,
  TDD,
  TIA,
  TII,
  TIN,
  TMA,
  TXS,
}

OpcInfo :: struct {
  instr: Instr,
  adr: AdrMode,
  ref_cyc: u8,  // refence cyles, can be 0 if unknonw
  extra: u8,    // for these instruction with numbers c:
}

@(private="file")
hand_opcode_table := [256]OpcInfo{
  0x03 = {.ST0,.Imm,4,0},
  0x05 = {.ORA,.Zpg,4,0},
  0x06 = {.ASL,.Zpg,6,0},
  0x0E = {.ASL,.Abs,7,0},
  0x10 = {.BPL,.Rel,2,0},
  0x13 = {.ST1,.Imm,4,0},
  0x16 = {.ASL,.Zpx,6,0},
  0x18 = {.CLC,.Imp,2,0},
  0x1A = {.INC,.Acc,2,0},
  0x1E = {.ASL,.Abx,7,0},
  0x20 = {.JSR,.Abs,7,0},
  0x23 = {.ST2,.Imm,4,0},
  0x38 = {.SEC,.Imp,2,0},
  0x3A = {.DEC,.Acc,2,0},
  0x40 = {.RTI,.Imp,7,0},
  0x43 = {.TMA,.Imp,4,0},
  0x44 = {.BSR,.Rel,8,0},
  0x48 = {.PHA,.Imp,3,0},
  0x4C = {.JMP,.Abs,4,0},
  0x53 = {.TAM,.Imp,5,0},
  0x54 = {.CSL,.Imp,0,0}, // csl cyc count is unknown
  0x58 = {.CLI,.Imp,2,0},
  0x5A = {.PHY,.Imp,3,0},
  0x60 = {.RTS,.Imp,7,0},
  0x62 = {.CLA,.Imp,2,0},
  0x64 = {.STZ,.Zpg,4,0},
  0x65 = {.ADC,.Zpg,4,0},
  0x68 = {.PLA,.Imp,4,0},
  0x69 = {.ADC,.Imm,2,0},
  0x73 = {.TII,.Imp,0,0}, // blk transfer instr can be variable
  0x78 = {.SEI,.Imp,2,0},
  0x7A = {.PLY,.Imp,4,0},
  0x80 = {.BRA,.Rel,4,0},
  0x82 = {.CLX,.Imp,2,0},
  0x85 = {.STA,.Zpg,4,0},
  0x8D = {.STA,.Abs,5,0},
  0x8E = {.STX,.Abs,5,0},
  0x91 = {.STA,.Ziy,7,0},
  0x9A = {.TXS,.Imp,2,0},
  0x9C = {.STZ,.Abs,5,0},
  0xA2 = {.LDX,.Imm,2,0},
  0xA5 = {.LDA,.Zpg,4,0},
  0xA9 = {.LDA,.Imm,2,0},
  0xAD = {.LDA,.Abs,5,0},
  0xB0 = {.BCS,.Rel,2,0},
  0xBD = {.LDA,.Abx,5,0},
  0xC0 = {.CPY,.Imm,2,0},
  0xC2 = {.CLY,.Imp,2,0},
  0xC3 = {.TDD,.Imp,0,0}, // blk transfer instr can be variable
  0xC4 = {.CPY,.Zpg,4,0},
  0xC5 = {.CMP,.Zpg,4,0},
  0xC6 = {.DEC,.Zpg,6,0},
  0xC8 = {.INY,.Imp,2,0},
  0xC9 = {.CMP,.Imm,2,0},
  0xCA = {.DEX,.Imp,2,0},
  0xCC = {.CPY,.Abs,5,0},
  0xD0 = {.BNE,.Rel,2,0},
  0xD3 = {.TIN,.Imp,0,0}, // blk transfer instr can be variable
  0xD4 = {.CSH,.Imp,0,0}, // csh cyc count is unknown
  0xD8 = {.CLD,.Imp,2,0},
  0xDA = {.PHX,.Imp,3,0},
  0xE3 = {.TIA,.Imp,0,0}, // blk transfer instr can be variable
  0xE5 = {.SBC,.Zpg,4,0},
  0xE8 = {.INX,.Imp,2,0},
  0xF0 = {.BEQ,.Rel,2,0},
  0xF3 = {.TAI,.Imp,0,0}, // blk instruction
  0xFA = {.PLX,.Imp,4,0},
}

init_opcode :: proc(opc: u8) -> OpcInfo {
  hi, lo := opc>>4, opc&0xF
  if lo == 0xF {
    // BBR 0xF0 - 0xF7
    if hi >= 0x0 && hi <= 0x7 {
      return {.BBR,.Zpr,6,hi-0x0}
    }

    // BBS 0xF8 - 0xFF
    if hi >= 0x8 && hi <= 0xF {
      return {.BBS,.Zpr,6,hi-0x8}
    }
  } else if lo == 0x7 {
    // RMB 0x70 - 0x77
    if hi >= 0x0 && hi <= 0x7 {
      return {.RMB,.Zpg,7,hi-0x0}
    }

    // SMB 0x78 - 0x7F
    if hi >= 0x8 && hi <= 0xF {
      return {.SMB,.Zpg,7,hi-0x8}
    }
  }

  return hand_opcode_table[opc] 
}

init_opcode_table :: proc() -> (opcodes: [256]OpcInfo) {
  for i in 0..<256 {
    opcodes[i] = init_opcode(cast(u8)i)
  }
  return
}