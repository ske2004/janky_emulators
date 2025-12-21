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

VdcMwr :: bit_field u16 {
  padding0:  uint | 4,
  tmap_size: uint | 3,
}

tmap_size_table := [8]struct{w, h: uint} {
  {32,  32},
  {64,  32},
  {128, 32},
  {128, 32},
  {32,  64},
  {64,  64},
  {128, 64},
  {128, 64},
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
  mwr: VdcMwr,

  satb_dma_next_vblank: bool,

  w_buf: u16, // low byte for read/write, high byte trigger vdc_write_reg

  reg_selected: VdcReg,

  x, y: int
}

vram_sprite_size :: proc(spr: VramSprite) -> (uint, uint) {
  sizes := [4]uint{1, 2, 4, 4}
  return sizes[spr.flags.xsize], sizes[spr.flags.ysize]
}

vram_get_sprite :: proc(vram: ^Vram, i: uint) -> ^VramSprite {
  assert(i < 64)
  raw := uintptr(&vram.satb)
  return cast(^VramSprite)(raw+cast(uintptr)(i*size_of(VramSprite)))
}

vdc_rw_increment :: proc(vdc: ^Vdc) -> u16 {
  increment_table := [4]u16{1, 32, 64, 128}

  return increment_table[vdc.cr.rw_increment]
}

vdc_write_reg :: proc(vdc: ^Vdc, reg: VdcReg, val: u16, no_sideffect: bool) {
  log_instr_info("vdc write reg: %v %04X (%s)", reg, val, no_sideffect ? "low" : "high")

  switch reg {
  case .Mawr: vdc.mawr = val&0x7FFF
  case .Marr: vdc.marr = val&0x7FFF
  case .Vrw: panic("vrw write is special")
  case .Unused0, .Unused1:
  case .Cr: vdc.cr = cast(VdcCr)val; log.warnf("todo: vdc cr")
  case .Rcr: vdc.rcr = val&0x1FF
  case .Scrollx: vdc.scroll_x = val&0x1FF
  case .Scrolly: vdc.scroll_y = val&0x1FF
  case .Mwr:  vdc.mwr = cast(VdcMwr)val
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
  case .Cr: ret = cast(u16)vdc.cr; log.warnf("todo: vdc cr")
  case .Rcr: ret = vdc.rcr
  case .Scrollx: ret = vdc.scroll_x
  case .Scrolly: ret = vdc.scroll_y
  case .Mwr: return cast(u16)vdc.mwr
  case .Hsync, .Hdisp, .Vsync, .Vdisp, .Vend:
    log.warnf("todo: vdc sync register read")
  case .Dmactrl: ret = cast(u16)vdc.dmactrl
  case .Dmasrc: ret = vdc.dmasrc
  case .Dmadst: ret = vdc.dmadst
  case .Dmalen: ret = vdc.dmalen
  case .Dmaspr: ret = vdc.dmaspr
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

draw_sprite_tile :: proc(bus: ^Bus, vdc: ^Vdc, px, py: int, pal: uint, taddr: uint) {
  for x in 0..<uint(16) {
    for y in 0..<uint(16) {
      bit0 := (vdc.vram.vram[taddr+y+00]&(1<<(15-x))) > 0
      bit1 := (vdc.vram.vram[taddr+y+16]&(1<<(15-x))) > 0
      bit2 := (vdc.vram.vram[taddr+y+32]&(1<<(15-x))) > 0
      bit3 := (vdc.vram.vram[taddr+y+48]&(1<<(15-x))) > 0

      palidx := uint(bit0) | (uint(bit1)<<1) | (uint(bit2)<<2) | (uint(bit3)<<3)

      if palidx != 0 {
        color := bus.vce.pal[0x100+pal+palidx]
        plot_dot(bus, int(x)+px, int(y)+py, color) 
      }
    }
  }
}

vdc_dma_transfer :: proc(vdc: ^Vdc) {
  fmt.printf("DMA!\n")
  for vdc.dmalen > 0 {
    vdc.vram.vram[vdc.dmadst] = vdc.vram.vram[vdc.dmasrc]
    switch vdc.dmactrl.src_action {
      case .Inc: vdc.dmasrc += 1
      case .Dec: vdc.dmasrc -= 1
    }
    switch vdc.dmactrl.dst_action {
      case .Inc: vdc.dmadst += 1
      case .Dec: vdc.dmadst -= 1
    }
    vdc.dmalen -= 1
  }
  vdc.status.vram_dma_end = true
}

vdc_fill :: proc(bus: ^Bus, using vdc: ^Vdc) {
  bus.screen = {}

  for _, i in bus.screen {
    bus.screen[i] = bus.vce.pal[0]
  }

  for i in 0..<uint(64) {
    sprite := vram_get_sprite(&vram, i)
    if sprite.flags.foreground {
      continue
    }
    px, py := cast(int)sprite.x-32, cast(int)sprite.y-64
    w, h := vram_sprite_size(sprite^)
    pal := sprite.flags.pal*16
    taddr := cast(uint)sprite.tile<<5

    for y in 0..<h {
      for x in 0..<w {
        draw_sprite_tile(bus, vdc, px+cast(int)x*16, py+cast(int)y*16, pal, taddr+y*128+x*64)
      }
    }
  }

  tmap_size := tmap_size_table[mwr.tmap_size]

  for i in 0..<(tmap_size.w*tmap_size.h) {
    x, y := i%tmap_size.w*8, i/tmap_size.w*8
    tile := vram.vram[i]
    tile_pal := cast(uint)(tile >> 12)*16
    tile_def := cast(uint)(tile << 4)

    for tx in 0..<uint(8) {
      for ty in 0..<uint(8) {
        bit0 := (vdc.vram.vram[tile_def+ty+0]&(1<<(15-(tx+8)))) > 0
        bit1 := (vdc.vram.vram[tile_def+ty+0]&(1<<(15-(tx)))) > 0
        bit2 := (vdc.vram.vram[tile_def+ty+8]&(1<<(15-(tx+8)))) > 0
        bit3 := (vdc.vram.vram[tile_def+ty+8]&(1<<(15-(tx)))) > 0

        palidx := uint(bit0) | (uint(bit1)<<1) | (uint(bit2)<<2) | (uint(bit3)<<3)

        if palidx != 0 && tile_pal != 0 {
          color := bus.vce.pal[tile_pal+palidx]
          plot_dot(bus, int(x+tx), int(y+ty), color) 
        }
      }
    }
  }

  for i in 0..<uint(64) {
    sprite := vram_get_sprite(&vram, i)
    if !sprite.flags.foreground {
      continue
    }
    px, py := cast(int)sprite.x-32, cast(int)sprite.y-64
    w, h := vram_sprite_size(sprite^)
    pal := sprite.flags.pal*16
    taddr := cast(uint)sprite.tile<<5

    for y in 0..<h {
      for x in 0..<w {
        draw_sprite_tile(bus, vdc, px+cast(int)x*16, py+cast(int)y*16, pal, taddr+y*128+x*64)
      }
    }
  }
}

vdc_cyc := 0

vdc_cycle :: proc(bus: ^Bus, using vdc: ^Vdc) {
  vdc_cyc += 3

  // TODO: temporary
  if vdc_cyc >= 4 {
    vdc_cyc -= 4

    x += 1
    if x == 342 {
      if vdc.cr.scanline_int && int(vdc.rcr)-64 == y {
        log_instr_info("rcr!")
        vdc.status.scanline_happen = true
        bus_irq(bus, .Irq1)
      }

      x = 0
      y += 1
      if y == 263 {
        y = 0
        vdc_fill(bus, vdc)
        bus.vblank_occured = true
      }
    }

    if (x == 0 && y == 258 && vdc.cr.vblank_int) {
      if satb_dma_next_vblank || dmactrl.satb_dma_auto {
        // TODO: what if it overflows vram addr?
        mem.copy(&vram.satb, &vram.vram[dmaspr], size_of(vram.satb))
        satb_dma_next_vblank = false
        vdc.status.vram_to_satb_end = true
      }
      log_instr_info("vblank!")
      vdc.status.vblank_happen = true
      bus_irq(bus, .Irq1)
    }
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
      if vdc.reg_selected == .Dmalen {
        vdc_dma_transfer(vdc)
      }
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
