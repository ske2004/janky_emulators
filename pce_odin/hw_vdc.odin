package main

import "core:log"
import "core:mem"
import "core:fmt"

VdcReg :: enum {
  Mawr    = 0x00, // VDC memory write
  Marr    = 0x01, // VDC memory read
  Vrw     = 0x02, // VRAM read/write
  Unused0 = 0x03,
  Unused1 = 0x04,
  Cr      = 0x05, // control register
  Rcr     = 0x06, // raster compare register
  Scrollx = 0x07,
  Scrolly = 0x08,
  Mwr     = 0x09, // memory width register
  Hsync   = 0x0A,
  Hdisp   = 0x0B,
  Vsync   = 0x0C,
  Vdisp   = 0x0D,
  Vend    = 0x0E,
  Dmactrl = 0x0F,
  Dmasrc  = 0x10,
  Dmadst  = 0x11,
  Dmalen  = 0x12,
  Dmaspr  = 0x13,
}

VramSpriteFlags :: bit_field u16 {
  pal:        uint | 4,
  padding1:   uint | 3,
  foreground: bool | 1,
  xsize:      uint | 1,
  padding2:   uint | 2,
  xflip:      bool | 1,
  ysize:      uint | 2,
  padding3:   uint | 1,
  yflip:      bool | 1, 
}

VramSprite :: struct #packed {
  y: u16,
  x: u16,
  tile: u16,
  flags: VramSpriteFlags,
}

VdcStatus :: bit_field u8 {
  spr_coll_happen: bool | 1,
  spr_ovfw_happen: bool | 1,
  scanline_happen: bool | 1,
  vram_to_satb_end: bool | 1,
  vram_dma_end: bool | 1,
  vblank_happen: bool | 1
}

VdcCr :: bit_field u16 {
  spr_coll_int: bool | 1,
  spr_ovfw_int: bool | 1,
  scanline_int: bool | 1,
  vblank_int:   bool | 1,
  reserved0:    u8   | 2,
  spr_enable:   bool | 1,
  bkg_enable:   bool | 1,
  reserved1:    u8   | 2,
  rw_increment: u8   | 2,
}

DmaAction :: enum {
  Inc,
  Dec,
}

VdcDmactrl :: bit_field u16 {
  vram_to_satb_int: bool | 1, 
  vram_to_vram_int: bool | 1, 
  src_action: DmaAction | 1, 
  dst_action: DmaAction | 1, 
  satb_dma_auto:    bool | 1, // every vblank
}

Vram :: struct {
  vram: [0x8000]u16,
  satb: [0x100]u16,
}

Vdc :: struct {
  vram: Vram,

  status: VdcStatus,

  mawr: u16,
  marr: u16,
  cr: VdcCr,
  rcr: u16,
  scroll_x: u16,
  scroll_y: u16,
  dmactrl: VdcDmactrl,
  dmasrc: u16,
  dmadst: u16,
  dmalen: u16,
  dmaspr: u16, // satb source addr

  satb_dma_next_vblank: bool,

  w_buf: u16, // low byte for read/write, high byte trigger vdc_write_reg

  reg_selected: VdcReg,

  x, y: int
}

vram_sprite_size :: proc(spr: VramSprite) -> (int, int) {
  sizes := [4]int{16, 32, 64, 64}
  return sizes[spr.flags.xsize], sizes[spr.flags.ysize]
}

vram_get_sprite :: proc(vram: ^Vram, i: uint) -> ^VramSprite {
  assert(i < 64)
  raw := uintptr(&vram.satb)
  return cast(^VramSprite)(raw+cast(uintptr)(i*size_of(VramSprite)))
}

vdc_rw_increment :: proc(vdc: ^Vdc) -> u16 {
  if vdc.cr.rw_increment == 0 {
    return 1
  }

  return 1 << (vdc.cr.rw_increment + 4)
}

vdc_write_reg :: proc(vdc: ^Vdc, reg: VdcReg, val: u16, no_sideffect: bool) {
  log_instr_info("vdc write reg: %v %04X (%s)", reg, val, no_sideffect ? "low" : "high")

  switch reg {
  case .Mawr: vdc.mawr = val&0x7FFF
  case .Marr: vdc.marr = val&0x7FFF
  case .Vrw: panic("vrw write is special")
  case .Unused0, .Unused1:
  case .Cr:   vdc.cr = cast(VdcCr)val; log.warnf("todo: vdc cr")
  case .Rcr:  vdc.rcr = val&0x1FF
  case .Scrollx:  vdc.scroll_x = val&0x1FF
  case .Scrolly:  vdc.scroll_y = val&0x1FF
  case .Mwr:  log.warnf("todo: vdc mwr write")
  case .Hsync, .Hdisp, .Vsync, .Vdisp, .Vend:
    log.warnf("todo: vdc sync register write")
  case .Dmactrl: vdc.dmactrl = cast(VdcDmactrl)val
  case .Dmasrc: vdc.dmasrc = val
  case .Dmadst: vdc.dmadst = val
  case .Dmalen: vdc.dmalen = val
  case .Dmaspr: vdc.dmaspr = val; vdc.satb_dma_next_vblank = true;
  }
}

vdc_read_reg :: proc(vdc: ^Vdc, reg: VdcReg, no_sideffect: bool, no_log: bool = false) -> (ret: u16) {
  switch reg {
  case .Mawr: ret = vdc.mawr
  case .Marr: ret = vdc.marr
  case .Vrw:
    ret = vdc.vram.vram[vdc.marr]
    if !no_sideffect { vdc.marr += vdc_rw_increment(vdc); vdc.marr &= 0x7FFF }
  case .Unused0, .Unused1:
  case .Cr:   ret = cast(u16)vdc.cr; log.warnf("todo: vdc cr")
  case .Rcr:  ret = vdc.rcr
  case .Scrollx:  ret = vdc.scroll_x
  case .Scrolly:  ret = vdc.scroll_y
  case .Mwr:  log.warnf("todo: vdc mwr read")
  case .Hsync, .Hdisp, .Vsync, .Vdisp, .Vend:
    log.warnf("todo: vdc sync register read")
  case .Dmactrl: ret = cast(u16)vdc.dmactrl
  case .Dmasrc:  ret = vdc.dmasrc
  case .Dmadst:  ret = vdc.dmadst
  case .Dmalen:  ret = vdc.dmalen
  case .Dmaspr:  ret = vdc.dmaspr
  }
    

  if !no_log {
    log_instr_info("vdc read reg: %v %04X (%s)", reg, ret, no_sideffect ? "low" : "high")
  }

  return
}

plot_dot :: proc(bus: ^Bus, x, y: int, color: Rgb333) {
  if x<0 || y<0 || x>=256 || y>=224 do return

  bus.screen[x+y*256] = color
}

vdc_fill :: proc(bus: ^Bus, using vdc: ^Vdc) {
  bus.screen = {}

  if satb_dma_next_vblank || dmactrl.satb_dma_auto {
    // TODO: what if it overflows vram addr?
    mem.copy(&vram.satb, &vram.vram[dmaspr], size_of(vram.satb))
    satb_dma_next_vblank = false
  }

  for i in 0..<uint(64) {
    sprite := vram_get_sprite(&vram, i)
    px, py := cast(int)sprite.x-32, cast(int)sprite.x-64
    w, h := vram_sprite_size(sprite^)

    for x in 0..<w {
      for y in 0..<h {
        plot_dot(bus, x+px, y+py, {r=7, g=7, b=7}) 
      }
    }
  }
}

vdc_cycle :: proc(bus: ^Bus, using vdc: ^Vdc) {
  x += 3
  if x == 342 {
    x = 0
    y += 1
    if y == 263 {
      y = 0
      vdc_fill(bus, vdc)
      bus.vblank_occured = true
    }
  }

  if (x == 0 && y == 258 && vdc.cr.vblank_int) {
    log_instr_info("vblank!")
    vdc.status.vblank_happen = true
    bus_irq(bus, .Irq1)
  }
}

vdc_write :: proc(bus: ^Bus, vdc: ^Vdc, addr: VdcAddrs, val: u8) {
  switch addr {
  case .Ctrl: vdc.reg_selected = cast(VdcReg)(val & 0x1F); if val > 19 do log.warnf("invalid reg selected %d", val)
  case .Unknown1:
  case .Data_lo:
    if vdc.reg_selected == .Vrw {
      vdc.vram.vram[vdc.mawr] = (vdc.vram.vram[vdc.mawr]&0xFF00) | cast(u16)val
    } else {
      vdc_write_reg(vdc, vdc.reg_selected, (vdc_read_reg(vdc, vdc.reg_selected, true, no_log=true)&0xFF00)|cast(u16)val, true)
    }
  case .Data_hi:
    if vdc.reg_selected == .Vrw {
      vdc.vram.vram[vdc.mawr] = (vdc.vram.vram[vdc.mawr]&0x00FF) | cast(u16)val<<8
      log_instr_info("vdc vrw write: %04X", vdc.vram.vram[vdc.mawr])
      vdc.mawr += vdc_rw_increment(vdc)
      vdc.mawr &= 0x7FFF
    } else {
      vdc_write_reg(vdc, vdc.reg_selected, (vdc_read_reg(vdc, vdc.reg_selected, true, no_log=true)&0xFF)|(cast(u16)val << 8), false)
    }
  }
}

vdc_read :: proc(bus: ^Bus, vdc: ^Vdc, addr: VdcAddrs) -> (ret: u8) {
  switch addr {
  case .Ctrl: ret = cast(u8)vdc.status; vdc.status = cast(VdcStatus)0
  case .Unknown1:
  case .Data_lo: ret = cast(u8)(vdc_read_reg(vdc, vdc.reg_selected, true)&0xFF)
  case .Data_hi: ret = cast(u8)(vdc_read_reg(vdc, vdc.reg_selected, false)>>8)
  }

  return
}
