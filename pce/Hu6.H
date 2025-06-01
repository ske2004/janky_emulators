/* HuC6280 cpu */
#pragma once

#include "Basics.H"
#include "Clk.H"

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
  U0 *userdata;
  virtual U8 Read(U32 addr);
  virtual U8 Write(U32 addr, U8 data);
};

struct Hu6
{
  U8 a,x,y;
  U16 pc,sp;
  U8 flags;
  U8 mpr[8]; // Memory Page Registers
  Bus *bus;
  Clk *clk;
};

auto Hu6New(Bus &bus, Clk &clk) -> Hu6
{
  auto cpu=Hu6();
  cpu.flags=FLG_INT|FLG_BRK;
  cpu.pc=BNK_SEL(7);
  cpu.bus=&bus;
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
  ClkNext(*cpu.clk);
  return byte;
}

auto Hu6WriteMem(Hu6 &cpu, U16 addr, U8 data) -> void
{
  cpu.bus->Write(Hu6MemMap(cpu, addr), data);
  ClkNext(*cpu.clk);
}

auto Hu6Run(Hu6 &cpu) -> void
{
  U8 opc = Hu6ReadMem(cpu, cpu.pc++);

  switch (opc) {
  }
}
