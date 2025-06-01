/* HuC6280 cpu */
/* TODO: Handle decimal mode */
#pragma once

#include "Basics.H"
#include "Clk.H"
#include "Dbg.H"
#include <cassert>

/* cpu starts at bank 7 */
#define BNK_START 7
#define BNK_MASK  0x1FFF
#define BNK_SHIFT 13
#define BNK_SEL(bnk) (bnk<<BNK_SHIFT)

/* flags */
#define FLG_CAR 0x01 // carry
#define FLG_ZER 0x02 // zero
#define FLG_INT 0x04 // interrupt
#define FLG_DEC 0x08 // decimal
#define FLG_BRK 0x10 // break
#define FLG_MEM 0x20 // memory
#define FLG_OFW 0x40 // overflow
#define FLG_NEG 0x80 // negative

/* addressing modes */
#define ADR_IMP 0  // implied
#define ADR_IMM 1  // immediate
#define ADR_ZPG 2  // zero page
#define ADR_ZPX 3  // zero page, x
#define ADR_ZPY 4  // zero page, y
#define ADR_ZPR 5  // zero page, relative
#define ADR_ZPI 6  // (zero page indirect)
#define ADR_ZXI 7  // (zero page x), indirect
#define ADR_ZIY 8  // (zero page indirect), y
#define ADR_ABS 9  // absolute
#define ADR_ABX 10 // absolute, x
#define ADR_ABY 11 // absolute, y
#define ADR_ABI 12 // (absolute indirect)
#define ADR_AXI 13 // (absolute, x)
#define ADR_REL 14 // relative
#define ADR_IZP 15 // immediate zeropage
#define ADR_IZX 16 // immediate zeropage, x
#define ADR_IAB 17 // immediate absolute
#define ADR_IAX 18 // immediate absolute, x
#define ADR_ACC 19 // accumulator

struct Bus
{
  virtual auto Read(U32 addr) -> U8 = 0;
  virtual auto Write(U32 addr, U8 data) -> U0 = 0;
};

struct Hu6
{
  U8 a,x,y;
  U16 pc,sp;
  U8 flags;
  U8 mpr[8]; // Memory Page Registers
  Bus *bus;
  Clk *clk;
  U16 instr_pc; // PC relative to instr start
  Bool fast;
};

struct Hu6Adr
{
  Hu6 *cpu;
  Bool isimm;
  U8 imm;
  U16 src, dst;
};

struct Opc
{
  CStr instr_name;
  U64 addr_mode;
  void (*Handler)(Hu6 &cpu, Opc opc);
  U8 opc;
  U8 ref_cyc; // reference cycle count, 0 for when unknown like blk transfer ops
};

auto Hu6New(Bus *bus, Clk &clk) -> Hu6
{
  auto cpu=Hu6();
  cpu.flags=FLG_INT|FLG_BRK;
  cpu.pc=BNK_SEL(7);
  cpu.bus=bus;
  cpu.clk=&clk;
  return cpu;
}

auto Hu6MemMap(Hu6 &cpu, U16 addr) -> U32
{
  U16 low_addr=addr&BNK_MASK;
  U16 bnk_sel=addr>>BNK_SHIFT;
  return cpu.mpr[bnk_sel]<<BNK_SHIFT|low_addr;
}

auto Hu6ReadMem(Hu6 &cpu, U16 addr) -> U8
{
  U8 byte = cpu.bus->Read(Hu6MemMap(cpu, addr));
  ClkCycle(*cpu.clk);
  return byte;
}

auto Hu6WriteMem(Hu6 &cpu, U16 addr, U8 data) -> void
{
  cpu.bus->Write(Hu6MemMap(cpu, addr), data);
  ClkCycle(*cpu.clk);
}

auto Hu6SetNZ(Hu6 &cpu, I8 data) -> U8
{
  if (data==0) cpu.flags|=FLG_ZER;
  else cpu.flags&=~FLG_ZER;
  if (data<0) cpu.flags|=FLG_NEG;
  else cpu.flags&=~FLG_NEG;
  return data;
}

auto Hu6AdrNew(Hu6 &cpu, U64 addr_mode) -> Hu6Adr
{
  Hu6Adr adr={};
  adr.cpu=&cpu;

  switch (addr_mode) {
  case ADR_IMP: DbgFail("CPU: AdrNew on IMP adr mode o_o");
  case ADR_IMM: adr.isimm=true; adr.imm=Hu6ReadMem(cpu, cpu.pc++); break;
  case ADR_ZPG: adr.src=Hu6ReadMem(cpu, cpu.pc++); break;
  default: DbgFail("CPU: AdrNew on unknown adr mode %02X PC=%04X CYC=%08X\n", addr_mode, cpu.instr_pc, cpu.clk->counter);
  }

  return adr;
}

auto AdrRead(Hu6Adr &adr) -> U8
{
  if (adr.isimm) {
    return adr.imm;
  }

  return Hu6ReadMem(*adr.cpu, adr.src);
}

auto AdrWrite(Hu6Adr &adr, U8 data) -> void
{
  if (adr.isimm) {
    DbgFail("CPU: AdrWrite on IMM adr mode o_o");
  }

  Hu6WriteMem(*adr.cpu, adr.src, data);
}

auto InstrUNK(Hu6 &cpu, Opc opc)
{
  DbgFail("CPU: unk opcode %02X PC=%04X CYC=%08X\n", opc.opc, cpu.instr_pc, cpu.clk->counter);
}

auto InstrSEI(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.flags|= FLG_INT;
  cpu.flags&=~FLG_MEM;
}

auto InstrCSH(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.fast = true;
  cpu.flags&=~FLG_MEM;
}

auto InstrCSL(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.fast = false;
  cpu.flags&=~FLG_MEM;
}

auto InstrCLD(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.flags&=~FLG_DEC;
  cpu.flags&=~FLG_MEM;
}

auto InstrLDA(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr = Hu6AdrNew(cpu, opc.addr_mode);
  cpu.a=Hu6SetNZ(cpu, AdrRead(adr));
  cpu.flags&=~FLG_DEC;
  cpu.flags&=~FLG_MEM;
}

auto Hu6GetOpc(U8 opc)->Opc
{
  switch (opc) {
  case 0x54: return{"CSL",ADR_IMP,InstrCSL,opc,0}; // csl cyc count is unknown
  case 0x78: return{"SEI",ADR_IMP,InstrSEI,opc,2};
  case 0xA9: return{"LDA",ADR_IMM,InstrLDA,opc,2};
  case 0xD4: return{"CSH",ADR_IMP,InstrCSH,opc,0}; // csh cyc count is unknown
  case 0xD8: return{"CLD",ADR_IMP,InstrCLD,opc,2};
  }

  return{"UNK",ADR_IMP,InstrUNK,opc,0};
}

auto Hu6Run(Hu6 &cpu) -> void
{
  while (ClkIsGoing(*cpu.clk)) {
    cpu.instr_pc = cpu.pc;
    U64 cyc_pre=cpu.clk->counter;
    U8 opc_id=Hu6ReadMem(cpu, cpu.pc++);
    Opc opc=Hu6GetOpc(opc_id);
    opc.Handler(cpu, opc);
    U64 cyc_post=cpu.clk->counter;
    if (opc.ref_cyc!=0 && cyc_post-cyc_pre!=opc.ref_cyc) {
      DbgWarn("CPU: cyc mismatch %s (exp:%d,got:%d) PC=%04X CYC=%08X\n",
              opc.instr_name, opc.ref_cyc, cyc_post-cyc_pre, cpu.instr_pc, cyc_post-cyc_pre);
    }
  }
}
