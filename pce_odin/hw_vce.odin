package main

Rgb333 :: bit_field u16 {
  r: uint | 3,
  g: uint | 3,
  b: uint | 3,
}

VceFreq :: enum {
  Mhz5,
  Mhz7,
}

VceCtrl :: bit_field u16 {
  freq: VceFreq | 1
}

Vce :: struct {
  pal: [0x200]Rgb333,
  pal_index: u16,
  ctrl: VceCtrl
}

vce_read :: proc(using vce: ^Vce, addr: VceAddrs) -> u8 {
  switch addr {
  case .Ctrl_lo: return cast(u8)cast(u16)vce.ctrl&0xFF
  case .Ctrl_hi: return 0
  case .PalSelect_lo: return cast(u8)(pal_index&0xFF)
  case .PalSelect_hi: return cast(u8)((pal_index>>8)&1)
  case .PalColor_lo:  return cast(u8)(cast(u16)pal[pal_index]&0xFF)
  case .PalColor_hi:  return cast(u8)((cast(u16)pal[pal_index]>>8)&1)
  case .Unknown6, .Unknown7: return 0xFF
  }

  unreachable()
}


vce_write :: proc(using vce: ^Vce, addr: VceAddrs, val: u8) {
  switch addr {
  case .Ctrl_lo: vce.ctrl = cast(VceCtrl)val
  case .Ctrl_hi: return
  case .PalSelect_lo: pal_index = (pal_index&0x100) | cast(u16)val
  case .PalSelect_hi: pal_index = (pal_index&0xFF) | (((cast(u16)val)<<8)&0x100)
  case .PalColor_lo:  pal[pal_index] = cast(Rgb333)((cast(u16)pal[pal_index]&0x100) | cast(u16)val)
  case .PalColor_hi:  pal[pal_index] = cast(Rgb333)((cast(u16)pal[pal_index]&0xFF) | ((cast(u16)val<<8)&0x100))
  case .Unknown6, .Unknown7: return
  }
} 
