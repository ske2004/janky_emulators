#pragma once

#include "Basics.H"
#include "Dbg.H"

struct Vdc
{
  enum class Reg
  {
    MAWR   , /* VDC memory write */
    MARR   , /* VDC memory read */
    VRW    , /* VRAM read/write */
    UNUSED0,
    UNUSED1,
    CR     , /* control register */
    RCR    , /* raster compare register */
    SCROLLX,
    SCROLLY,
    MWR    , /* memory width register */
    HDR    ,
  };

  enum class Adr
  {
    REG_SEL, /* select register */
    UNUSED0,
    DATA_LO,
    DATA_HI,
  };

  U8 selected_register {};

  struct {
    U16 cr;
    U16 rcr;
    U16 scrollx;
    U16 scrolly;
    U16 mwr;
  } regs {};

  auto GetRegPtr(Reg reg, Bool hi) -> U8*
  {
    /* won't work on big endian but whatever */
    switch (reg) {
    case Reg::CR:      return &((uint8_t*)(&regs.cr))[hi]; break;
    case Reg::RCR:     return &((uint8_t*)(&regs.rcr))[hi]; break;
    case Reg::SCROLLX: return &((uint8_t*)(&regs.scrollx))[hi]; break;
    case Reg::SCROLLY: return &((uint8_t*)(&regs.scrolly))[hi]; break;
    case Reg::MWR:     return &((uint8_t*)(&regs.mwr))[hi]; break;
    default: Dbg::Err("VDC/GRP", "failed"); break;
    }

    return nullptr;
  }

  auto Write(Adr adr, U8 data) -> void
  {
    switch (adr) {
    case Adr::REG_SEL:
      Dbg::Info("VDC/W", "WR REG_SEL: $%02X", data);
      selected_register = data&0x1F; /* mask out last 5 bits */
      break;
    case Adr::UNUSED0:
      break;
    case Adr::DATA_LO:
    case Adr::DATA_HI:
      {
        U8 *ptr = GetRegPtr(Reg(selected_register), adr==Adr::DATA_HI);
        if (ptr) {
          Dbg::Info("VDC/W", "REG %d (+%d): $%02X", selected_register, adr==Adr::DATA_HI, data);
          *ptr = data;
        } else {
          Dbg::Err("VDC/W", "REG %d (+%d): $%02X FAILED", selected_register, adr==Adr::DATA_HI, data);
        }
      }
      break;
    default:
      Dbg::Info("VDC/W", "unhandled %d", adr);
      break;
    }
  }

  auto Read(Adr adr) -> U8
  {
    switch (adr) {
    case Adr::REG_SEL: return 0; break; /* TODO: VDC status */
    case Adr::UNUSED0: return 0; break;
    case Adr::DATA_LO:
    case Adr::DATA_HI:
      {
        U8 *ptr = GetRegPtr(Reg(selected_register), adr==Adr::DATA_HI);
        if (ptr) {
          Dbg::Info("VDC/R", "REG %d (+%d): $%02X", selected_register, adr==Adr::DATA_HI, *ptr);
          return *ptr;
        } else {
          Dbg::Err("VDC/R", "REG %d (+%d): FAILED", selected_register, adr==Adr::DATA_HI);
        }
      }
      break;
    default: Dbg::Info("VDC/R", "unhandled %d", adr); break;
    }
    return 0xFF;
  }
};