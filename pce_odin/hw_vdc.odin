package main

import "core:log"
import "core:mem"
import "core:fmt"
import "core:container/small_array"

VDC_Reg :: enum {
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
  DMActrl = 0x0F,
  DMAsrc  = 0x10,
  DMAdst  = 0x11,
  DMAlen  = 0x12,
  DMAspr  = 0x13,
}

VDC_DMA_State :: enum {
  None,
  Read_Src,
  Write_Dst,
}

VRAM_Sprite_Flags :: bit_field u16 {
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

VRAM_Sprite :: struct #packed {
  y: u16,
  x: u16,
  tile: u16,
  flags: VRAM_Sprite_Flags,
}

VDC_Status :: bit_field u8 {
  spr_coll_happen: bool | 1,
  spr_ovfw_happen: bool | 1,
  scanline_happen: bool | 1,
  vram_to_satb_end: bool | 1,
  vram_dma_end: bool | 1,
  vblank_happen: bool | 1,
}

VDC_Cr :: bit_field u16 {
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

VDC_DMA_Action :: enum {
  Inc = 0,
  Dec = 1,
}

VDC_DMA_Ctrl :: bit_field u16 {
  vram_to_satb_int: bool | 1, 
  vram_to_vram_int: bool | 1, 
  src_action: VDC_DMA_Action | 1, 
  dst_action: VDC_DMA_Action | 1, 
  satb_dma_auto:    bool | 1, // every vblank
}

VDC_Mwr :: bit_field u16 {
  padding0:  uint | 4,
  tmap_size: uint | 3,
}

tmap_size_table := [8][2]uint{
  {32,  32},
  {64,  32},
  {128, 32},
  {128, 32},
  {32,  64},
  {64,  64},
  {128, 64},
  {128, 64},
}

VRAM :: struct {
  vram: [0x8000]u16,
  satb: [0x100]u16,
}

VDC :: struct {
  vram: VRAM,

  status: VDC_Status,

  mawr: u16,
  marr: u16,
  cr: VDC_Cr,
  rcr: u16,
  scroll_x: u16,
  scroll_y: u16,
  dmactrl: VDC_DMA_Ctrl,
  dmasrc: u16,
  dmadst: u16,
  dmalen: u16,
  dmaspr: u16, // satb source addr
  mwr: VDC_Mwr,

  satb_dma_next_vblank: bool,

  commit_vblank_satb_dma_in_clocks: int,

  reg_selected: VDC_Reg,

  dma_state: VDC_DMA_State,
  dma_buf: u16,

  x, y: int,
  ty: u16,
}

vram_sprite_dims :: proc(spr: VRAM_Sprite) -> (uint, uint) {
  sizes := [4]uint{16, 32, 64, 64}
  return sizes[spr.flags.xsize], sizes[spr.flags.ysize]
}

vram_sprite_size :: proc(spr: VRAM_Sprite) -> (uint, uint) {
  sizes := [4]uint{1, 2, 4, 4}
  return sizes[spr.flags.xsize], sizes[spr.flags.ysize]
}

vram_get_sprite :: proc(vram: ^VRAM, i: uint) -> ^VRAM_Sprite {
  assert(i < 64)
  raw := uintptr(&vram.satb)
  return cast(^VRAM_Sprite)(raw+cast(uintptr)(i*size_of(VRAM_Sprite)))
}

vdc_rw_increment :: proc(vdc: ^VDC) -> u16 {
  increment_table := [4]u16{1, 32, 64, 128}

  return increment_table[vdc.cr.rw_increment]
}

plot_dot :: proc(bus: ^Bus, x, y: int, color: RGB333) {
  if x<0 || y<0 || x>=256 || y>=224 do return

  bus.screen[x+y*256] = color
}

draw_sprite_tile_strip :: proc(bus: ^Bus, vdc: ^VDC, px, py: int, pal: uint, taddr: uint, sub_y: uint, flip: bool) {
  for x in 0..<uint(16) {
    xx := flip ? x : 15-x
    bit0 := (vdc.vram.vram[taddr+sub_y+00]&(1<<xx)) > 0
    bit1 := (vdc.vram.vram[taddr+sub_y+16]&(1<<xx)) > 0
    bit2 := (vdc.vram.vram[taddr+sub_y+32]&(1<<xx)) > 0
    bit3 := (vdc.vram.vram[taddr+sub_y+48]&(1<<xx)) > 0

    palidx := uint(bit0) | (uint(bit1)<<1) | (uint(bit2)<<2) | (uint(bit3)<<3)

    if palidx != 0 {
      color := bus.vce.pal[0x100+pal+palidx]
      plot_dot(bus, int(x)+px, int(sub_y)+py, color) 
    }
  }
}

draw_sprite_strip :: proc(bus: ^Bus, vdc: ^VDC, sprites: []VRAM_Sprite, y: int, fg: bool) {
  if !vdc.cr.spr_enable {
    return
  }

  for spr, i in sprites {
    if spr.flags.foreground == fg {
      px, py := cast(int)spr.x-32, cast(int)spr.y-64
      yo := (cast(uint)y - cast(uint)py)
      tile_y, sub_y := yo / 16, yo % 16
      w, h := vram_sprite_size(spr)
      pal := spr.flags.pal*16
      taddr := cast(uint)spr.tile<<5
      
      yy := tile_y
      if spr.flags.yflip {
        yy = (h-1)-tile_y
        sub_y = 15-sub_y
      }

      for x in 0..<w {
        xx := x
        if spr.flags.xflip {
          xx = (w-1)-x
        }

        draw_sprite_tile_strip(
          bus   = bus,
          vdc   = vdc,
          px    = px+cast(int)x*16,
          py    = py+cast(int)tile_y*16,
          pal   = pal,
          taddr = (taddr+yy*128+xx*64)&0x7FFF,
          sub_y = sub_y,
          flip  = spr.flags.xflip,
        )
      }
    }
  }
}

vdc_draw_scanline :: proc(bus: ^Bus, vdc: ^VDC, y: int) {
  sprites: small_array.Small_Array(16, VRAM_Sprite)

  for i in 0..<256 {
    bus.screen[i+y*256] = bus.vce.pal[0]
  }

  for i in 0..<uint(64) {
    sprite := vram_get_sprite(&vdc.vram, i)
    w, h := vram_sprite_dims(sprite^)

    if y >= cast(int)sprite.y-64 && y < cast(int)sprite.y-64+cast(int)h {
      if !small_array.append_elem(&sprites, sprite^) {
        // todo: spr overflow
      }
    }
  }

  draw_sprite_strip(bus, vdc, small_array.slice(&sprites), y, false)
 
  if vdc.cr.bkg_enable {

    tmap_size := tmap_size_table[vdc.mwr.tmap_size]
    for x in 0..<uint(256) {
      ax, ay := cast(int)x + cast(int)vdc.scroll_x, cast(int)vdc.ty
      tx, sx := (ax/8)%cast(int)tmap_size.x, cast(uint)ax%8
      ty, sy := (ay/8)%cast(int)tmap_size.y, cast(uint)ay%8
      if tx < 0 do tx += cast(int)tmap_size.x
      if ty < 0 do ty += cast(int)tmap_size.y
      tile := vdc.vram.vram[cast(uint)tx+cast(uint)ty*tmap_size.x]
      tile_pal := cast(uint)(tile >> 12)*16
      tile_def := cast(uint)(tile << 4)&0x7FFF

      bit0 := (vdc.vram.vram[tile_def+sy+0]&(1<<(15-(sx+8)))) > 0
      bit1 := (vdc.vram.vram[tile_def+sy+0]&(1<<(15-(sx)))) > 0
      bit2 := (vdc.vram.vram[tile_def+sy+8]&(1<<(15-(sx+8)))) > 0
      bit3 := (vdc.vram.vram[tile_def+sy+8]&(1<<(15-(sx)))) > 0

      palidx := uint(bit0) | (uint(bit1)<<1) | (uint(bit2)<<2) | (uint(bit3)<<3)

      if palidx != 0 {
        color := bus.vce.pal[tile_pal+palidx]
        bus.screen[x+uint(y)*256] = color
      }
    }
  }

  draw_sprite_strip(bus, vdc, small_array.slice(&sprites), y, true)
}

vdc_cyc := 0

vdc_cycle :: proc(bus: ^Bus, using vdc: ^VDC) {
  test := [1+2]int{}

  vdc_cyc += 3

  // TODO: temporary
  if vdc_cyc >= 4 {
    vdc_cyc -= 4

    switch vdc.dma_state {
    case .None:
    case .Read_Src:
      vdc.dmalen -= 1
      vdc.dma_buf = vdc.vram.vram[vdc.dmasrc]
      switch vdc.dmactrl.src_action {
      case .Inc: vdc.dmasrc += 1
      case .Dec: vdc.dmasrc -= 1
      }
      vdc.dmasrc &= 0x7FFF
      vdc.dma_state = .Write_Dst
    case .Write_Dst:
      vdc.vram.vram[vdc.dmadst] = vdc.dma_buf
      switch vdc.dmactrl.dst_action {
      case .Inc: vdc.dmadst += 1
      case .Dec: vdc.dmadst -= 1
      }
      vdc.dmadst &= 0x7FFF
      vdc.dma_state = .Read_Src

      if vdc.dmalen == 0 {
        vdc.dma_state = .None

        vdc.status.vram_dma_end = true
        if vdc.dmactrl.vram_to_vram_int {
          bus_irq(bus, .IRQ1)
        }
      }
    }

    if vdc.commit_vblank_satb_dma_in_clocks > 0 {
      vdc.commit_vblank_satb_dma_in_clocks -= 1

      if vdc.commit_vblank_satb_dma_in_clocks == 0 {
        vdc.status.vram_to_satb_end = true
        if vdc.dmactrl.vram_to_satb_int {
          bus_irq(bus, .IRQ1)
        }
      }
    }

    x += 1
    if x == 342 {
      ty += 1
      if y < 224 {
        vdc_draw_scanline(bus, vdc, y)
      } 
      if y < 224 && vdc.cr.scanline_int && int(vdc.rcr)-64 == y {
        log_instr_info("rcr!")
        vdc.status.scanline_happen = true
        bus_irq(bus, .IRQ1)
      }

      x = 0
      y += 1

      if y == 263 {
        y = 0
        ty = scroll_y
        bus.vblank_occured = true
      }
    }

    if (x == 0 && y == 258 && vdc.cr.vblank_int) {
      log_instr_info("vblank!")
      vdc.status.vblank_happen = true
      bus_irq(bus, .IRQ1)
    }

    if x == 0 && y == 260 {
      if satb_dma_next_vblank || dmactrl.satb_dma_auto {
        // TODO: what if it overflows vram addr?
        mem.copy(&vram.satb, &vram.vram[dmaspr], size_of(vram.satb))
        satb_dma_next_vblank = false
        vdc.commit_vblank_satb_dma_in_clocks = 0x100
      }
    }
  }
}

vdc_write_reg :: proc(vdc: ^VDC, reg: VDC_Reg, val: u16, no_sideffect: bool) {
  log_instr_info("vdc write reg: %v %04X (%s)", reg, val, no_sideffect ? "low" : "high")

  switch reg {
  case .Mawr: vdc.mawr = val&0x7FFF
  case .Marr: vdc.marr = val&0x7FFF
  case .Vrw: panic("vrw write is special")
  case .Unused0, .Unused1:
  case .Cr: vdc.cr = cast(VDC_Cr)val; log.warnf("todo: vdc cr")
  case .Rcr: vdc.rcr = val&0x1FF
  case .Scrollx: vdc.scroll_x = val&0x3FF
  case .Scrolly: vdc.scroll_y = val&0x1FF; vdc.ty = val&0x1FF
  case .Mwr:  vdc.mwr = cast(VDC_Mwr)val
  case .Hsync, .Hdisp, .Vsync, .Vdisp, .Vend:
    log.warnf("todo: vdc sync register write")
  case .DMActrl: vdc.dmactrl = cast(VDC_DMA_Ctrl)val
  case .DMAsrc: vdc.dmasrc = val&0x7FFF
  case .DMAdst: vdc.dmadst = val&0x7FFF
  case .DMAlen: vdc.dmalen = val
  case .DMAspr: vdc.dmaspr = val; vdc.satb_dma_next_vblank = true
  }
}

vdc_read_reg :: proc(vdc: ^VDC, reg: VDC_Reg, no_sideffect: bool, no_log: bool = false) -> (ret: u16) {
  switch reg {
  case .Mawr: ret = vdc.mawr
  case .Marr: ret = vdc.marr
  case .Vrw:
    ret = vdc.vram.vram[vdc.marr]
    if !no_sideffect { vdc.marr += vdc_rw_increment(vdc); vdc.marr &= 0x7FFF }
  case .Unused0, .Unused1:
  case .Cr: ret = cast(u16)vdc.cr; log.warnf("todo: vdc cr")
  case .Rcr: ret = vdc.rcr
  case .Scrollx: ret = cast(u16)vdc.scroll_x
  case .Scrolly: ret = cast(u16)vdc.scroll_y
  case .Mwr: return cast(u16)vdc.mwr
  case .Hsync, .Hdisp, .Vsync, .Vdisp, .Vend:
    log.warnf("todo: vdc sync register read")
  case .DMActrl: ret = cast(u16)vdc.dmactrl
  case .DMAsrc: ret = vdc.dmasrc
  case .DMAdst: ret = vdc.dmadst
  case .DMAlen: ret = vdc.dmalen
  case .DMAspr: ret = vdc.dmaspr
  }
    

  if !no_log {
    log_instr_info("vdc read reg: %v %04X (%s)", reg, ret, no_sideffect ? "low" : "high")
  }

  return
}

vdc_write :: proc(bus: ^Bus, vdc: ^VDC, addr: VDC_Addrs, val: u8) {
  switch addr {
  case .Ctrl: vdc.reg_selected = cast(VDC_Reg)(val & 0x1F); if val > 19 do log.warnf("invalid reg selected %d", val)
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
      log_instr_info("vdc vrw write: %04X %04X", vdc.mawr, vdc.vram.vram[vdc.mawr])
      vdc.mawr += vdc_rw_increment(vdc)
      vdc.mawr &= 0x7FFF
    } else {
      vdc_write_reg(vdc, vdc.reg_selected, (vdc_read_reg(vdc, vdc.reg_selected, true, no_log=true)&0xFF)|(cast(u16)val << 8), false)
      if vdc.reg_selected == .DMAlen {
        fmt.printf("DMA!\n")
        vdc.dma_state = .Read_Src
      }
    }
  }
}

vdc_read :: proc(bus: ^Bus, vdc: ^VDC, addr: VDC_Addrs) -> (ret: u8) {
  switch addr {
  case .Ctrl: ret = cast(u8)vdc.status; vdc.status = cast(VDC_Status)0
  case .Unknown1:
  case .Data_lo: ret = cast(u8)(vdc_read_reg(vdc, vdc.reg_selected, true)&0xFF)
  case .Data_hi:
    ret = cast(u8)(vdc_read_reg(vdc, vdc.reg_selected, false)>>8)
    if vdc.reg_selected == .Marr {
      vdc.marr += vdc_rw_increment(vdc)
      vdc.marr &= 0x7FFF
    }
  }

  return
}
