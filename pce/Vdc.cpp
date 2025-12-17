#include "Vdc.H"

void Vdc::PlotDot(int x, int y, Vce::Color value)
{
  if (x >= 256 || y >= 224 || x < 0 || y < 0) {
    return;
  }

  screen[x+y*256] = value&0x1FF; // just in case
}

// not how it works
void Vdc::FillScreen()
{
  memset(screen, 0, sizeof screen);
  
  if (satb_dma_next_vblank) {
    // TODO: what if it overflows vram addr?
    memcpy(vram.sprites, vram.vram+satb_dma, sizeof vram.sprites);
    satb_dma_next_vblank = false;
  }

  for (int i=0; i<64; i++) {
    VramSprite sprite = vram.GetSprite(i);
    U16 px = ((I16)sprite.x)-32, py = ((I16)sprite.y)-64;
    U16 w, h;
    sprite.Size(w, h);

    for (int x=0; x<w; x++) {
      for (int y=0; y<h; y++) {
        PlotDot(x+px, y+py, 0x1FF);
      }
    }
  }

  out.SetRGB16(screen);
}

auto Vdc::Cycle() -> U0
{
  // TODO: assuming 5mhz 
  x += 1;
  if (x == 342) {
    x = 0;
    y += 1;
    if (y == 263) {
      y = 0;
      out.NextFrame();
    }
  }

  if (x == 0 && y == 258) {
    Dbg::Info("VDC", "VBLANK!!1");
    irq.Request(Irq::IRQ1);
    FillScreen();
  }
}

auto Vdc::RegWrite(Reg reg, U8 data, Bool lo) -> void
{
  Dbg::Info("VDC/W", lo ? "RegWrite: XX%02X" : "RegWrite: %02XXX", data);

  switch (reg) {
    case Reg::CR:
    case Reg::RCR:
      Dbg::Todo("VDC/W", "CR");
      break;
    case Reg::MARR:
      PiecewiseWrite16(addr, data, lo);
      rwmode = 0;
      break;
    case Reg::MAWR:
      PiecewiseWrite16(addr, data, lo);
      rwmode = 1;
      break;
    case Reg::VRW:
      Dbg::Info("VDC/W", "VRAM write! %02X %d", data, lo);
      PiecewiseWrite16(vram.vram[addr], data, lo);
      if (rwmode == 1) {
        addr++;
      }
      break;
    case Reg::DMASPR:
      PiecewiseWrite16(satb_dma, data, lo);
      satb_dma_next_vblank = true;
      break;
    default:
      Dbg::Warn("VDC/W", "Unhandled register %d (%s)", reg, _vdc::reg_names[(int)reg]);
  }
}

auto Vdc::Write(Adr adr, U8 data) -> void
{
  // Dbg::Info("VDC/W", "VDC write %d %s $%02X", (int)adr, _vdc::adr_names[(int)adr], data);

  switch (adr) {
  case Adr::REG_SEL:
    Dbg::Info("VDC/W", "SELECT REGISTER: $%02X (%s)", data, _vdc::reg_names[data]);
    selected_register = data&0x1F; /* mask out last 5 bits */
    break;
  case Adr::UNUSED0:
    break;
  case Adr::DATA_LO:
  case Adr::DATA_HI:
    RegWrite(Reg(selected_register), data, adr == Adr::DATA_LO);
    break;
  default:
    Dbg::Info("VDC/W", "unhandled %d", adr);
    break;
  }
}

auto Vdc::Read(Adr adr) -> U8
{
  Dbg::Info("VDC/R", "VDC read %d %s", (int)adr, _vdc::adr_names[(int)adr]);

  switch (adr) {
  case Adr::REG_SEL:
    irq.Ack(Irq::IRQ1);
    return 0;
    break; /* TODO: VDC status */
  case Adr::UNUSED0: return 0; break;
  case Adr::DATA_LO:
  case Adr::DATA_HI:
    
    break;
  default: Dbg::Info("VDC/R", "unhandled %d", adr); break;
  }
  return 0xFF;
}
