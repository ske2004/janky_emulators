#include "Basics.H"
#include "Bin.H"
#include "Hu6.H"
#include "Dbg.H"
#include "Arr.H"
#include "Vce.H"
#include "Vdc.H"
#include <cstdio>

/* PC-Engine phys memory map, denoted by pg number.
 * PC-Engine uses a 21bit phys memory bus. So, do bsr 13 to get pg number.
 *
 * $00:$7F ROM
 * $80:$F7 Unused (ret $FF)
 * $F8:$FB RAM (only 1 pg, $2000 bytes, rest is mirrored)
 * $FC:$FE Unused (ret $FF)
 * $FF:$FF Hardware pg
 */

struct Pce : IBus
{
  PceBin &rom;
  Clk clk;
  Hu6 cpu;
  Vce vce;
  Vdc vdc;
  Arr<U8, 0x2000> ram; // TODO: what is init ram state?

  Pce(PceBin &rom);

  auto Read(U32 addr) -> U8 override;
  auto Write(U32 addr, U8 data) -> U0 override;
  auto Run()->U0;
};

auto Pce::Read(U32 addr) -> U8
{
  U32 pg = ADR_BNK(addr);
  addr = addr&BNK_MASK;
  if (pg>=0x00 && pg<=0x7F) return rom.Get(addr);
  if (pg>=0xF8 && pg<=0xFB) return ram[addr];
  if (pg==0xFF) {
    clk.Cycle();
    if (addr>=0x000 && addr<=0x399) {
      return vdc.Read(Vdc::Adr(addr%4));
    } else if (addr>=0x400 && addr<=0x7FF) {
      return vce.Read(Vce::Adr(addr%8));
    } else {
      Dbg::Err("PCE: Read from hardware pg at $%04X", addr);
    }
  }
  return 0xFF;
}

auto Pce::Write(U32 addr, U8 data) -> U0
{
  U32 pg = ADR_BNK(addr);
  addr = addr&BNK_MASK;
  if (pg>=0x00 && pg<=0x7F) rom.Set(addr, data);
  if (pg>=0xF8 && pg<=0xFB) ram[addr] = data;
  if (pg==0xFF) {
    clk.Cycle();
    if (addr>=0x000 && addr<=0x399) {
      vdc.Write(Vdc::Adr(addr%4), data);
    } else if (addr>=0x400 && addr<=0x7FF) {
      vce.Write(Vce::Adr(addr%8), data);
    } else {
      Dbg::Err("PCE: Write to hardware pg $%04X", addr);
    }
  }
}

Pce::Pce(PceBin &rom) : 
  rom(rom),
  ram({}),
  clk([this](){ /* TODO: aux cycle for vdc and cpu */}),
  cpu(*this, this->clk)
{
}

auto Pce::Run()->U0
{
  Dbg::Info("PCE: run");
  cpu.Run();
}

auto main(I32 argc, CStr argv[]) -> I32
{
  if (argc != 2) {
    printf("%s <rom_file>\n", argv[0]);
    return 1;
  }

  Dbg::Info("MAIN: Loading ROM %s", argv[1]);
  try {
    PceBin bin(argv[1]);
    Pce pce(bin);
    pce.Run();
  } catch (CStr err) {
    Dbg::Fail("Fatal error: %s\n", err);
  }
}
