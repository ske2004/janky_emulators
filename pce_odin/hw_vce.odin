package main

import "core:fmt"

RGB333 :: bit_field u16 {
  b: uint | 3,
  r: uint | 3,
  g: uint | 3,
}

VCE_Freq :: enum {
  Mhz5,
  Mhz7,
}

VCE_Ctrl :: bit_field u16 {
  freq: VCE_Freq | 1,
}

VCE :: struct {
  pal: [0x200]RGB333,
  pal_index: u16,
  ctrl: VCE_Ctrl,
}

vce_read :: proc(vce: ^VCE, addr: VCE_Addrs) -> u8 {
  switch addr {
  case .Ctrl_lo: return cast(u8)vce.ctrl
  case .Ctrl_hi: return 0
  case .PalSelect_lo: return cast(u8)vce.pal_index
  case .PalSelect_hi: return cast(u8)((vce.pal_index>>8)&1)
  case .PalColor_lo:  return cast(u8)(cast(u16)vce.pal[vce.pal_index]&0xFF)
  case .PalColor_hi:
    ret := cast(u8)((cast(u16)vce.pal[vce.pal_index]>>8)&1)
    vce.pal_index += 1
    vce.pal_index &= 0x1FF
    return ret
  case .Unknown6, .Unknown7: return 0xFF
  }

  unreachable()
}


vce_write :: proc(vce: ^VCE, addr: VCE_Addrs, val: u8) {
  switch addr {
  case .Ctrl_lo: vce.ctrl = cast(VCE_Ctrl)val
  case .Ctrl_hi: return
  case .PalSelect_lo: vce.pal_index = (vce.pal_index&0x100) | cast(u16)val
  case .PalSelect_hi: vce.pal_index = (vce.pal_index&0xFF) | (((cast(u16)val)<<8)&0x100)
  case .PalColor_lo:  vce.pal[vce.pal_index] = cast(RGB333)((cast(u16)vce.pal[vce.pal_index]&0x100) | cast(u16)val)
  case .PalColor_hi:
    vce.pal[vce.pal_index] = cast(RGB333)((cast(u16)vce.pal[vce.pal_index]&0xFF) | ((cast(u16)val<<8)&0x100))
    log_instr_info("vce val %v\n", vce.pal[vce.pal_index])
    vce.pal_index += 1
    vce.pal_index &= 0x1FF
  case .Unknown6, .Unknown7: return
  }
} 
