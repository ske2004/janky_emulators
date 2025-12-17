/* HuC6280 cpu */
/* TODO: Handle decimal mode */
/* TODO: Blot instrs have incorrect cyc counts */
/* TODO: Handle mem flag */
#pragma once

#include "Basics.H"
#include "Clk.H"
#include "Dbg.H"
#include "IBus.H"
#include "Irq.H"
#include <cassert>
#include <cstdio>

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

#define MEM_VECIRQ2  0xFFF6
#define MEM_VECIRQ1  0xFFF8
#define MEM_VECTIMER 0xFFFA
#define MEM_VECNMI   0xFFFC

struct Hu6
{
  U8 a=0,x=0,y=0;
  U16 pc=0;
  U8 sp=0;
  U8 flags=0;
  U8 mpr[8]={};   // Memory Page Registers
  IBus &bus;
  Clk &clk;
  U16 instr_pc=0; // PC relative to instr start
  Bool fast=false;
  Irq &irq;

  Hu6(IBus &_bus, Clk &_clk, Irq &_irq) : bus(_bus), clk(_clk), irq(_irq)
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
  U8 extra;
};