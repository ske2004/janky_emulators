/* HuC6280 cpu */
/* TODO: Handle decimal mode */
/* TODO: Blot instrs have incorrect cyc counts */
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

#define ZPG_START 0x2000
#define STK_START 0x2100

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

auto Hu6Cycle(Hu6 &cpu)
{
  if (cpu.fast)
    ClkCycle(*cpu.clk);
  else
    for (U8 i=0; i<4; i++) // TODO: 4 is approximate?
      ClkCycle(*cpu.clk);
}

auto Hu6ReadMem(Hu6 &cpu, U16 addr) -> U8
{
  U8 byte = cpu.bus->Read(Hu6MemMap(cpu, addr));
  Hu6Cycle(cpu);
  return byte;
}

auto Hu6WriteMem(Hu6 &cpu, U16 addr, U8 data) -> void
{
  cpu.bus->Write(Hu6MemMap(cpu, addr), data);
  Hu6Cycle(cpu);
}

auto Hu6SetNZ(Hu6 &cpu, I8 data) -> U8
{
  if (data==0) cpu.flags|=FLG_ZER;
  else cpu.flags&=~FLG_ZER;
  if (data<0) cpu.flags|=FLG_NEG;
  else cpu.flags&=~FLG_NEG;
  return data;
}

auto StkRead(Hu6 &cpu) -> U8
{
  return Hu6ReadMem(cpu, STK_START+cpu.sp);
}

auto StkWrite(Hu6 &cpu, U8 val)
{
  return Hu6WriteMem(cpu, STK_START+cpu.sp, val);
}

auto StkPush(Hu6 &cpu, U8 val)
{
  Hu6ReadMem(cpu, STK_START+cpu.sp); // dummy
  return Hu6WriteMem(cpu, STK_START+cpu.sp--, val);
}

auto StkPop(Hu6 &cpu) -> U8
{
  Hu6ReadMem(cpu, STK_START+((cpu.sp+1)&0xFF)); // dummy
  return Hu6ReadMem(cpu, STK_START+ ++cpu.sp);
}

auto Hu6AdrNew(Hu6 &cpu, U64 addr_mode) -> Hu6Adr
{
  Hu6Adr adr={};
  adr.cpu=&cpu;

  switch (addr_mode) {
  case ADR_IMP: DbgFail("CPU: AdrNew on IMP adr mode o_o");
  case ADR_IMM: adr.isimm=true; adr.imm=Hu6ReadMem(cpu, cpu.pc++); break;
  case ADR_ZPG: adr.src=Hu6ReadMem(cpu, cpu.pc++)+ZPG_START; break;
  default: DbgFail("CPU: AdrNew on unknown adr mode %02X PC=%04X CYC=%08X\n", addr_mode, cpu.instr_pc, cpu.clk->counter);
  }

  return adr;
}

auto AdrRead(Hu6Adr &adr) -> U8
{
  if (adr.isimm) {
    DbgInfo("CPU: AdrRead IMM: %02X", adr.imm);
    return adr.imm;
  }

  U8 val = Hu6ReadMem(*adr.cpu, adr.src);
  DbgInfo("CPU: AdrRead SRC:$04X VAL:%02X", adr.src, val);
  return val;
}

auto AdrWrite(Hu6Adr &adr, U8 data) -> void
{
  if (adr.isimm) {
    DbgFail("CPU: AdrWrite on IMM adr mode o_o");
  }

  Hu6ReadMem(*adr.cpu, adr.src); // dummy
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

auto InstrLDX(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr = Hu6AdrNew(cpu, opc.addr_mode);
  cpu.x=Hu6SetNZ(cpu, AdrRead(adr));
  cpu.flags&=~FLG_MEM;
}

auto InstrLDA(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr = Hu6AdrNew(cpu, opc.addr_mode);
  cpu.a=Hu6SetNZ(cpu, AdrRead(adr));
  cpu.flags&=~FLG_MEM;
}

auto InstrTAM(Hu6 &cpu, Opc opc)
{
  U8 mask=Hu6ReadMem(cpu, cpu.pc++);
  for (U8 i=0; i<8; i++) {
    if (mask&(1<<i))
      cpu.mpr[i]=cpu.a;
  }
  cpu.flags&=~FLG_MEM;
}

auto InstrTXS(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.sp=cpu.x;
  cpu.flags&=~FLG_MEM;
}

auto InstrSTZ(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  AdrWrite(adr, 0);
  cpu.flags&=~FLG_MEM;
}

auto InstrTII(Hu6 &cpu, Opc opc)
{
  U64 clk_start=cpu.clk->counter;

  U16 src=(Hu6ReadMem(cpu, cpu.pc++)<<0)|(Hu6ReadMem(cpu, cpu.pc++)<<8);
  U16 dst=(Hu6ReadMem(cpu, cpu.pc++)<<0)|(Hu6ReadMem(cpu, cpu.pc++)<<8);
  U16 len=(Hu6ReadMem(cpu, cpu.pc++)<<0)|(Hu6ReadMem(cpu, cpu.pc++)<<8);
  U16 len_orig=len;

  StkPush(cpu, cpu.y);
  StkPush(cpu, cpu.a);
  StkWrite(cpu, cpu.x);

  U64 clk_tran_start=cpu.clk->counter;

  while (--len) {
    Hu6WriteMem(cpu, dst++, Hu6ReadMem(cpu, src++));
  }

  U64 clk_tran_end=cpu.clk->counter;

  cpu.x=StkRead(cpu);
  cpu.a=StkPop(cpu);
  cpu.y=StkPop(cpu);

  cpu.flags&=~FLG_MEM;

  U64 cyc_total=cpu.clk->counter-clk_start;
  U64 cyc_tran=clk_tran_end-clk_tran_start;
  U64 cyc_notran=cyc_total-cyc_tran+1; // account for opc cycle

  DbgWarn("CPU: BLK TII: cyctot:%d, cyctrn:%d, cycntr:%d, len:%04hx", cyc_total, cyc_tran, cyc_notran, len_orig);
}

auto InstrJSR(Hu6 &cpu, Opc opc)
{
  U16 addr=Hu6ReadMem(cpu, cpu.pc++)|Hu6ReadMem(cpu, cpu.pc++)<<8;

  StkPush(cpu, cpu.pc>>8);
  StkPush(cpu, cpu.pc&0xff);

  DbgInfo("CPU: JSR: pc:%04hx, addr:%04hx", cpu.pc, addr);

  cpu.pc=addr;
  cpu.flags&=~FLG_MEM;
}

auto InstrSTA(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  AdrWrite(adr, cpu.a);
  cpu.flags&=~FLG_MEM;
}

auto InstrRTS(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.pc=StkPop(cpu)|StkPop(cpu)<<8;
  DbgInfo("CPU: RTS: pc:%04hx", cpu.pc);
  cpu.pc++;
  cpu.flags&=~FLG_MEM;
}

auto Hu6GetOpc(U8 opc)->Opc
{
  switch (opc) {
  case 0x20:return{"JSR",ADR_ABS,InstrJSR,opc,7};
  case 0x53:return{"TAM",ADR_IMP,InstrTAM,opc,5};
  case 0x54:return{"CSL",ADR_IMP,InstrCSL,opc,0}; // csl cyc count is unknown
  case 0x60:return{"RTS",ADR_IMP,InstrRTS,opc,7};
  case 0x64:return{"STZ",ADR_ZPG,InstrSTZ,opc,4};
  case 0x73:return{"TII",ADR_IMP,InstrTII,opc,0}; // blk transfer instr can be variable
  case 0x78:return{"SEI",ADR_IMP,InstrSEI,opc,2};
  case 0x85:return{"STA",ADR_ZPG,InstrSTA,opc,4};
  case 0x9A:return{"TXS",ADR_IMP,InstrTXS,opc,2};
  case 0xA2:return{"LDX",ADR_IMM,InstrLDX,opc,2};
  case 0xA9:return{"LDA",ADR_IMM,InstrLDA,opc,2};
  case 0xD4:return{"CSH",ADR_IMP,InstrCSH,opc,0}; // csh cyc count is unknown
  case 0xD8:return{"CLD",ADR_IMP,InstrCLD,opc,2};
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
    DbgInfo("CPU: %s", opc.instr_name);
    opc.Handler(cpu, opc);
    U64 cyc_post=cpu.clk->counter;
    if (!cpu.fast) opc.ref_cyc*=4;
    if (opc.ref_cyc!=0 && cyc_post-cyc_pre!=opc.ref_cyc) {
      DbgWarn("CPU: cyc mismatch %s (exp:%d,got:%d) PC=%04X CYC=%08X",
              opc.instr_name, opc.ref_cyc, cyc_post-cyc_pre, cpu.instr_pc, cyc_post-cyc_pre);
    }
  }
}
