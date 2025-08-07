/* HuC6280 cpu */
/* TODO: Handle decimal mode */
/* TODO: Blot instrs have incorrect cyc counts */
/* TODO: Handle mem flag */
#pragma once

#include "Basics.H"
#include "Clk.H"
#include "Dbg.H"
#include <cassert>

/* cpu starts at bank 7 */
#define BNK_START 7
#define BNK_MASK  0x1FFF
#define BNK_SHIFT 13
#define BNK_SEL(bnk) ((bnk)<<BNK_SHIFT)
#define ADR_BNK(adr) ((adr)>>BNK_SHIFT)

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

struct IBus
{
  virtual auto Read(U32 addr) -> U8 = 0;
  virtual auto Write(U32 addr, U8 data) -> U0 = 0;
};

struct Hu6
{
  U8 a=0,x=0,y=0;
  U16 pc;
  U8 sp=0;
  U8 flags;
  U8 mpr[8]={};   // Memory Page Registers
  IBus &bus;
  Clk &clk;
  U16 instr_pc=0; // PC relative to instr start
  Bool fast=false;

  Hu6(IBus &_bus, Clk &_clk) : bus(_bus), clk(_clk)
  {
    flags=FLG_INT|FLG_BRK;
    pc=BNK_SEL(7);
  }

  auto Run() -> void;
};

struct Hu6Adr
{
  Hu6 *cpu;
  Bool isimm;
  Bool isa;
  U8 imm;
  U16 adr; /* effective address */
};

struct Opc
{
  CStr instr_name;
  U64 addr_mode;
  void (*Handler)(Hu6 &cpu, Opc opc);
  U8 opc;
  U8 ref_cyc; // reference cycle count, 0 for when unknown like blk transfer ops
};

auto Hu6Carry(Hu6 &cpu)
{
  return (cpu.flags & FLG_CAR) > 0;
}

auto Hu6SetFlag(Hu6 &cpu, U8 flag, Bool set)
{
  if (set)
    cpu.flags|=1<<flag;
  else
    cpu.flags&=~(1<<flag);
}

auto Hu6MemMap(Hu6 &cpu, U16 addr) -> U32
{
  U16 low_addr=addr&BNK_MASK;
  U16 bnk_sel=addr>>BNK_SHIFT;
  return cpu.mpr[bnk_sel]<<BNK_SHIFT|low_addr;
}

auto Hu6Cycle(Hu6 &cpu)
{
  if (cpu.fast) {
    for (U8 i=0; i<3; i++) 
      cpu.clk.Cycle();
  } else {
    for (U8 i=0; i<12; i++) 
      cpu.clk.Cycle();
  }
}

auto Hu6ReadMem(Hu6 &cpu, U16 addr) -> U8
{
  U8 byte = cpu.bus.Read(Hu6MemMap(cpu, addr));
  Hu6Cycle(cpu);
  return byte;
}

auto Hu6WriteMem(Hu6 &cpu, U16 addr, U8 data) -> void
{
  cpu.bus.Write(Hu6MemMap(cpu, addr), data);
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
  case ADR_IMP: break;
  case ADR_IMM: adr.isimm=true; adr.imm=Hu6ReadMem(cpu, cpu.pc++); break;
  case ADR_ZPG: adr.adr=Hu6ReadMem(cpu, cpu.pc++)+ZPG_START; break;
  case ADR_ABS: adr.adr=Hu6ReadMem(cpu, cpu.pc++); adr.adr|=Hu6ReadMem(cpu, cpu.pc++)<<8; break;
  case ADR_ABX: adr.adr=Hu6ReadMem(cpu, cpu.pc++); adr.adr|=Hu6ReadMem(cpu, cpu.pc++)<<8; adr.adr+=cpu.x; break;
  case ADR_ACC: /* dumdum */ Hu6ReadMem(cpu, cpu.pc); adr.isa=true; adr.imm=cpu.a; break;
  default: Dbg::Fail("CPU/A", "AdrNew on unknown adr mode %02X PC=%04X CYC=%08llX\n", (int)addr_mode, cpu.instr_pc, cpu.clk.counter); break;
  }

  return adr;
}

auto AdrRead(Hu6Adr &adr) -> U8
{
  if (adr.isimm) {
    Dbg::Info("CPU/A", "AdrRead IMM: %02X", adr.imm);
    return adr.imm;
  }

  if (adr.isa) {
    Dbg::Info("CPU/A", "AdrRead A: %02X", adr.imm);
    return adr.imm;
  }

  U8 val = Hu6ReadMem(*adr.cpu, adr.adr);
  Dbg::Info("CPU/A", "AdrRead SRC:$%04X VAL:%02X", adr.adr, val);
  return val;
}

auto AdrWrite(Hu6Adr &adr, U8 data) -> void
{
  if (adr.isimm) {
    Dbg::Fail("CPU/A", "AdrWrite on IMM adr mode o_o");
  }

  if (adr.isa) {
    Dbg::Info("CPU/A", "AdrWrite A VAL:%02X", data);
    adr.cpu->a = data;
    return;
  }

  Dbg::Info("CPU/A", "AdrWrite DST:%04X VAL:%02X", adr.adr, data);
  Hu6ReadMem(*adr.cpu, adr.adr); // dummy
  Hu6WriteMem(*adr.cpu, adr.adr, data);
}

auto InstrUNK(Hu6 &cpu, Opc opc)
{
  Dbg::Fail("CPU/A", "UNK opcode %02X PC=%04X CYC=%08llX\n", opc.opc, cpu.instr_pc, cpu.clk.counter);
}

auto InstrSEI(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.flags|= FLG_INT;
}

auto InstrCSH(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.fast = true;
}

auto InstrCSL(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.fast = false;
}

auto InstrCLD(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.flags&=~FLG_DEC;
}

auto InstrLDX(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr = Hu6AdrNew(cpu, opc.addr_mode);
  cpu.x=Hu6SetNZ(cpu, AdrRead(adr));
}

auto InstrLDA(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr = Hu6AdrNew(cpu, opc.addr_mode);
  cpu.a=Hu6SetNZ(cpu, AdrRead(adr));
}

auto InstrTAM(Hu6 &cpu, Opc opc)
{
  U8 mask=Hu6ReadMem(cpu, cpu.pc++);
  Dbg::Info("CPU/I", "TAM: %02X=%02X", mask, cpu.a);
  for (U8 i=0; i<8; i++) {
    if (mask&(1<<i))
      cpu.mpr[i]=cpu.a;
  }
}

auto InstrTXS(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.sp=cpu.x;
}

auto InstrSTZ(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  AdrWrite(adr, 0);
}

auto InstrTII(Hu6 &cpu, Opc opc)
{
  U64 clk_start=cpu.clk.counter;

  U16 src=(Hu6ReadMem(cpu, cpu.pc++)<<0)|(Hu6ReadMem(cpu, cpu.pc++)<<8);
  U16 dst=(Hu6ReadMem(cpu, cpu.pc++)<<0)|(Hu6ReadMem(cpu, cpu.pc++)<<8);
  U16 len=(Hu6ReadMem(cpu, cpu.pc++)<<0)|(Hu6ReadMem(cpu, cpu.pc++)<<8);
  U16 len_orig=len;

  StkPush(cpu, cpu.y);
  StkPush(cpu, cpu.a);
  StkWrite(cpu, cpu.x);

  U64 clk_tran_start=cpu.clk.counter;

  while (--len) {
    Hu6WriteMem(cpu, dst++, Hu6ReadMem(cpu, src++));
  }

  U64 clk_tran_end=cpu.clk.counter;

  cpu.x=StkRead(cpu);
  cpu.a=StkPop(cpu);
  cpu.y=StkPop(cpu);

  U64 cyc_total=cpu.clk.counter-clk_start;
  U64 cyc_tran=clk_tran_end-clk_tran_start;
  U64 cyc_notran=cyc_total-cyc_tran+1; // account for opc cycle

  Dbg::Warn("CPU/I", "BLK TII: cyctot:%lld, cyctrn:%lld, cycntr:%lld, len:%04hX", cyc_total, cyc_tran, cyc_notran, len_orig);
}

auto InstrJSR(Hu6 &cpu, Opc opc)
{
  U16 addr=Hu6ReadMem(cpu, cpu.pc++)|Hu6ReadMem(cpu, cpu.pc++)<<8;

  /* subtract 1, jsr puts pc at byte 3 not next opcode */
  StkPush(cpu, (cpu.pc-1)>>8);
  StkPush(cpu, (cpu.pc-1)&0xff);

  Dbg::Info("CPU/I", "JSR: pc:$%04hX, addr:$%04hX", (cpu.pc-1), addr);

  cpu.pc=addr;
}

auto InstrSTA(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  AdrWrite(adr, cpu.a);
}

auto InstrRTS(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.pc=StkPop(cpu);
  cpu.pc|=StkPop(cpu)<<8;
  cpu.pc++;
  Dbg::Info("CPU/I", "RTS: pc:$%04hX", cpu.pc);
}

auto InstrCLX(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.x=0;
}

auto InstrCMP(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  U8 check = AdrRead(adr);
  U8 result = cpu.a-check;
  Hu6SetFlag(cpu, FLG_CAR, cpu.a>=check);
  Hu6SetFlag(cpu, FLG_ZER, cpu.a==check);
  Hu6SetFlag(cpu, FLG_NEG, result&0x80);
}

auto InstrBEQ(Hu6 &cpu, Opc opc)
{
  I8 rel=Hu6ReadMem(cpu, cpu.pc++);
  if (cpu.flags&FLG_ZER) {
    /* TODO: handle page crossing cycle */
    cpu.clk.Cycle();
    cpu.pc += rel;
  }
}

auto InstrBRA(Hu6 &cpu, Opc opc)
{
  I8 rel=Hu6ReadMem(cpu, cpu.pc++);
  /* TODO: handle page crossing cycle */
  cpu.clk.Cycle();
  cpu.pc += rel;
}

auto InstrINX(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.x++;
}

auto InstrCLA(Hu6 &cpu, Opc opc)
{
  Hu6ReadMem(cpu, cpu.pc);
  cpu.a=0;
}

auto InstrCLC(Hu6 &cpu, Opc opc) 
{
  Hu6ReadMem(cpu, cpu.pc);
  Hu6SetFlag(cpu, FLG_CAR, 0);
}

auto InstrADC(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  I8 val = AdrRead(adr);
  I16 a = (I16)cpu.a + (I16)val + (I16)Hu6Carry(cpu);
  I8 res = (I8)cpu.a + (I8)val + (I8)Hu6Carry(cpu);
  // god i hope this works
  Hu6SetFlag(cpu, FLG_CAR, (a>>8)&0x7F);
  Hu6SetFlag(cpu, FLG_NEG, res<0);
  Hu6SetFlag(cpu, FLG_ZER, res==0);
  Hu6SetFlag(cpu, FLG_OFW, a<-128 || a>127);
  cpu.a = res;
}

auto InstrINC(Hu6 &cpu, Opc opc)
{
  Hu6Adr adr=Hu6AdrNew(cpu, opc.addr_mode);
  I8 res = AdrRead(adr);
  res++;
  Hu6SetFlag(cpu, FLG_NEG, res<0);
  Hu6SetFlag(cpu, FLG_ZER, res==0);
  AdrWrite(adr, res);
}

auto Hu6GetOpc(U8 opc)->Opc
{
  switch (opc) {
  case 0x18:return{"CLC",ADR_IMP,InstrCLC,opc,2};
  case 0x1A:return{"INC",ADR_ACC,InstrINC,opc,2};
  case 0x20:return{"JSR",ADR_ABS,InstrJSR,opc,7};
  case 0x53:return{"TAM",ADR_IMP,InstrTAM,opc,5};
  case 0x54:return{"CSL",ADR_IMP,InstrCSL,opc,0}; // csl cyc count is unknown
  case 0x60:return{"RTS",ADR_IMP,InstrRTS,opc,7};
  case 0x62:return{"CLA",ADR_IMP,InstrCLA,opc,2};
  case 0x64:return{"STZ",ADR_ZPG,InstrSTZ,opc,4};
  case 0x65:return{"ADC",ADR_ZPG,InstrADC,opc,4};
  case 0x73:return{"TII",ADR_IMP,InstrTII,opc,0}; // blk transfer instr can be variable
  case 0x78:return{"SEI",ADR_IMP,InstrSEI,opc,2};
  case 0x80:return{"BRA",ADR_REL,InstrBRA,opc,4};
  case 0x82:return{"CLX",ADR_IMP,InstrCLX,opc,2};
  case 0x85:return{"STA",ADR_ZPG,InstrSTA,opc,4};
  case 0x8D:return{"STA",ADR_ABS,InstrSTA,opc,5};
  case 0x9A:return{"TXS",ADR_IMP,InstrTXS,opc,2};
  case 0xA2:return{"LDX",ADR_IMM,InstrLDX,opc,2};
  case 0xA9:return{"LDA",ADR_IMM,InstrLDA,opc,2};
  case 0xAD:return{"LDA",ADR_ABS,InstrLDA,opc,5};
  case 0xBD:return{"LDA",ADR_ABX,InstrLDA,opc,5};
  case 0xC9:return{"CMP",ADR_IMM,InstrCMP,opc,2};
  case 0xD4:return{"CSH",ADR_IMP,InstrCSH,opc,0}; // csh cyc count is unknown
  case 0xD8:return{"CLD",ADR_IMP,InstrCLD,opc,2};
  case 0xE8:return{"INX",ADR_IMP,InstrINX,opc,2};
  case 0xF0:return{"BEQ",ADR_REL,InstrBEQ,opc,2};
  }

  return{"UNK",ADR_IMP,InstrUNK,opc,0};
}


auto Hu6::Run() -> void
{
  while (this->clk.IsGoing()) {
    this->instr_pc = this->pc;
    U64 cyc_pre=this->clk.counter;
    U8 opc_id=Hu6ReadMem(*this, pc++);
    Opc opc=Hu6GetOpc(opc_id);
    Dbg::Info("CPU/I", "$%04X ($%06X) %s", this->instr_pc, Hu6MemMap(*this, this->instr_pc), opc.instr_name);
    /* TODO disable for mem flag set instruction */
    this->flags&=~FLG_MEM;
    opc.Handler(*this, opc);
    U64 cyc_post=this->clk.counter;
    I64 cyc_diff=cyc_post-cyc_pre-clk.extra_counter;
    clk.extra_counter=0; /* HACK */
    if (!this->fast) opc.ref_cyc*=4;
    if (opc.ref_cyc!=0 && cyc_diff!=opc.ref_cyc) {
      Dbg::Warn("CPU/I", "cyc mismatch %s (exp:%d,got:%lld) PC=%04X CYC=%08lld",
              opc.instr_name, opc.ref_cyc, cyc_diff, instr_pc, clk.counter);
    }
  }
}
