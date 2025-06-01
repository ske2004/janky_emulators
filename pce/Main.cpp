#include "Basics.H"
#include "Bin.H"
#include "Hu6.H"
#include "Dbg.H"
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

struct Pce : Bus
{
  Bin *rom;
  Clk clk;
  Hu6 cpu;
  U8 ram[0x2000]; // TODO: what is init ram state?

  auto Read(U32 addr) -> U8 override;
  auto Write(U32 addr, U8 data) -> U0 override;
};

auto Adr2Pg(U32 addr) -> U8
{
  return addr >> BNK_SHIFT;
}

auto Pce::Read(U32 addr) -> U8
{
  U32 pg = Adr2Pg(addr);
  if (pg>=0x00 && pg<=0x7F) return BinGetOob(rom, addr);
  if (pg>=0xF8 && pg<=0xFB) return ram[addr & BNK_MASK];
  if (pg>=0xFF && pg<=0xFF) {
    DbgErr("PCE: Read from hardware pg");
    return 0xFF;
  }
  return 0xFF;
}

auto Pce::Write(U32 addr, U8 data) -> U0
{
  U32 pg = Adr2Pg(addr);
  if (pg>=0x00 && pg<=0x7F) BinSetOob(rom, addr, data);
  if (pg>=0xF8 && pg<=0xFB) ram[addr & BNK_MASK] = data;
  if (pg>=0xFF && pg<=0xFF) DbgErr("PCE: Write to hardware pg");
}

auto PceNew(Bin *rom) -> Pce*
{
  auto pce=new Pce;
  pce->rom=rom;
  pce->clk=ClkNew(pce, [](void *userdata){
    auto pce = (Pce*)userdata;
    // TODO: aux cycle for vdc and apu
  });
  pce->cpu=Hu6New(pce, pce->clk);
  DbgInfo("PCE: new, rom: %p", rom);
  return pce;
}

auto PceDel(Pce &pce) -> U0
{
  DbgInfo("PCE: del");
  delete &pce;
}

auto PceRun(Pce &pce) -> U0
{
  DbgInfo("PCE: run");
  Hu6Run(pce.cpu);
}

auto main(I32 argc, CStr argv[]) -> I32
{
  if (argc != 2) {
    printf("%s <rom_file>\n", argv[0]);
    return 1;
  }

  DbgInfo("MAIN: Loading ROM %s", argv[1]);
  Bin *rom = BinRead(argv[1]);
  if (!rom) {
    DbgFail("Failed to load ROM\n");
  }
  Pce *pce = PceNew(rom);
  PceRun(*pce);
  PceDel(*pce);
  BinDel(rom);
}
