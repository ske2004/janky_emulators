package main

Rgb333 :: bit_field u16 {
  r: uint | 3,
  g: uint | 3,
  b: uint | 3,
}

Vce :: struct {
  pal: [0x200]Rgb333,
  pal_index: u16
}

vce_read :: proc(using vce: ^Vce, addr: VceAddrs) -> u8 {
  switch addr {
  case .Ctrl: unimplemented("vce ctrl r")
  case .PalSelect_lo: return cast(u8)(pal_index&0xFF)
  case .PalSelect_hi: return cast(u8)((pal_index>>8)&1)
  case .PalColor_lo:  return cast(u8)(cast(u16)pal[pal_index]&0xFF)
  case .PalColor_hi:  return cast(u8)((cast(u16)pal[pal_index]>>8)&1)
  case .Unknown5, .Unknown6, .Unknown7: return 0xFF
  }

  unreachable()
}


vce_write :: proc(using vce: ^Vce, addr: VceAddrs, val: u8) {
  switch addr {
  case .Ctrl: unimplemented("vce ctrl w")
  case .PalSelect_lo: pal_index = (pal_index&0x100) | cast(u16)val
  case .PalSelect_hi: pal_index = (pal_index&0xFF) | (((cast(u16)val)<<8)&0x100)
  case .PalColor_lo:  pal[pal_index] = cast(Rgb333)((cast(u16)pal[pal_index]&0x100) | cast(u16)val)
  case .PalColor_hi:  pal[pal_index] = cast(Rgb333)((cast(u16)pal[pal_index]&0xFF) | ((cast(u16)val<<8)&0x100))
  case .Unknown5, .Unknown6, .Unknown7: return
  }
} 
