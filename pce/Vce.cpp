#include "Vce.H"


auto Vce::Write(Adr adr, U8 data) -> void
{
  switch (adr) {
  case Adr::CTRL:
    Dbg::Info("VCE/W", "Write to CTRL: $%02X", data);
    break;
  case Adr::PALSELECT_LO:
    pal_index = (pal_index & 0x100) | data;
    break;
  case Adr::PALSELECT_HI:
    pal_index = (pal_index & 0xFF) | ((data&1)<<8);
    Dbg::Info("VCE", "pal_index: $%03X", pal_index);
    break;
  case Adr::PALCOLOR_LO:
    entries[pal_index] = (entries[pal_index] & 0x100) | data;
    break;
  case Adr::PALCOLOR_HI:
    entries[pal_index] = (entries[pal_index] & 0xFF) | ((data&1)<<8);
    break;
  default: Dbg::Info("VCE/W", "Write to unhandled %d", adr); break;
  }
}

auto Vce::Read(Adr adr) -> U8 
{
  switch (adr) {
  case Adr::CTRL:
    return 0x00;
    break;
  case Adr::PALSELECT_LO:
    return pal_index&0xFF;
    break;
  case Adr::PALSELECT_HI:
    return pal_index>>8;
    break;
  case Adr::PALCOLOR_LO:
    return entries[pal_index]&0xFF;
    break;
  case Adr::PALCOLOR_HI:
    return entries[pal_index]>>8;
    break;
  default: Dbg::Info("VCE/R", "Read from unhandled %d", adr); break;
  }

  return 0;
}
