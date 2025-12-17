#include "Pce.H"
#include "Dbg.H"

auto Pce::Read(U32 addr) -> U8
{
  U32 pg = ADR_BNK(addr);
  U16 mapped_addr = addr&BNK_MASK;
  if (pg>=0x00 && pg<=0x7F) return rom.Get(addr);
  if (pg>=0xF8 && pg<=0xFB) return ram[mapped_addr];
  if (pg==0xFF) {
    clk.ExtraCycle();
    return hwpage.Read(mapped_addr);
  }
  return 0xFF;
}

auto Pce::Write(U32 addr, U8 data) -> U0
{
  U32 pg = ADR_BNK(addr);
  addr = addr&BNK_MASK;
  if (pg>=0x00 && pg<=0x7F) return;
  if (pg>=0xF8 && pg<=0xFB) ram[addr] = data;
  if (pg==0xFF) {
    clk.ExtraCycle();
    hwpage.Write(addr, data);
  }
}

Pce::Pce(PceBin &rom, PceOut &out) : 
  irq(),
  rom(rom),
  ram({}),
  hwpage(vce, vdc, irq),
  clk([this](){ hwpage.Cycle(clk); }),
  vdc(vce, out, irq),
  cpu(*this, this->clk, irq)
{
}

auto Pce::Run()->U0
{
  Dbg::Info("PCE", "run");
  cpu.Run();
}


