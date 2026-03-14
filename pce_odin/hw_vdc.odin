// TODO: KILL
package main

import "core:log"
import "core:container/small_array"

VDC_Reg :: enum {
  Mawr    = 0x00, // VDC memory write
  Marr    = 0x01, // VDC memory read
  Vrw     = 0x02, // VRAM read/write
  Unused0 = 0x03, // TODO: https://pcedev.wordpress.com/2014/08/14/vdc-secret-registers/
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
  _:		      uint | 3,
  foreground: bool | 1,
  xsize:      uint | 1,
  _:			    uint | 2,
  xflip:      bool | 1,
  ysize:      uint | 2,
  _:			    uint | 1,
  yflip:      bool | 1, 
}

VRAM_Sprite :: struct #packed {
  y: u16,
  x: u16,
  tile: u16,
  flags: VRAM_Sprite_Flags,
}

VDC_Status :: bit_field u8 {
  spr_coll_happen:  bool | 1,
  spr_ovfw_happen:  bool | 1,
  scanline_happen:  bool | 1,
  vram_to_satb_end: bool | 1,
  vram_dma_end:     bool | 1,
  vblank_happen:    bool | 1,
  busy:             bool | 1,
}

VDC_Cr :: bit_field u16 {
  spr_coll_int: bool | 1,
  spr_ovfw_int: bool | 1,
  scanline_int: bool | 1,
  vblank_int:   bool | 1,
  reserved0:    u8   | 2,
  spr_enable:   bool | 1,
  bkg_enable:   bool | 1,
  reserved1:    u8   | 3,
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

  satb_dma_index: int,

  reg_selected: VDC_Reg,

  dma_state: VDC_DMA_State,
  dma_buf: u16,
  vram_buf: u16,
  out_width: uint,
  
  x, y: int,
  tx, ty: u16,
}

vram_sprite_dims :: proc(spr: VRAM_Sprite) -> (uint, uint) {
  sizes := [4]uint{16, 32, 64, 64}
  return sizes[spr.flags.xsize], sizes[spr.flags.ysize]
}

vram_sprite_size :: proc(spr: VRAM_Sprite) -> (int, int) {
  sizes := [4]int{1, 2, 4, 4}
  return sizes[spr.flags.xsize], sizes[spr.flags.ysize]
}

vram_get_sprite :: proc(vram: ^VRAM, i: uint) -> VRAM_Sprite {
  assert(i < 64)
  raw := uintptr(&vram.satb)
  sprite := (cast(^VRAM_Sprite)(raw+cast(uintptr)(i*size_of(VRAM_Sprite))))^
  sprite.x &= (1<<9)-1;
  sprite.y &= (1<<9)-1;
  sprite.tile &= (1<<10)-1;
  return sprite
}

vdc_rw_increment :: proc(vdc: ^VDC) -> u16 {
  increment_table := [4]u16{1, 32, 64, 128}

  return increment_table[vdc.cr.rw_increment]
}

plot_dot :: proc(bus: ^Bus, x, y: int, color: RGB333) {
  if x>=0 && y>=0 && x<512 && y<224 {
    bus.screen[x+y*512] = color
  }
}

draw_sprite_tile_strip :: proc(bus: ^Bus, vdc: ^VDC, px, py: int, pal: uint, taddr: uint, sub_y: uint, flip: bool, fg: bool) {
  for x in 0..<uint(16) {
    xx := flip ? x : 15-x
    bit0 := (vdc.vram.vram[(taddr+sub_y+00)&0x7FFF]&(1<<xx)) != 0
    bit1 := (vdc.vram.vram[(taddr+sub_y+16)&0x7FFF]&(1<<xx)) != 0
    bit2 := (vdc.vram.vram[(taddr+sub_y+32)&0x7FFF]&(1<<xx)) != 0
    bit3 := (vdc.vram.vram[(taddr+sub_y+48)&0x7FFF]&(1<<xx)) != 0

    palidx := uint(bit0) | (uint(bit1)<<1) | (uint(bit2)<<2) | (uint(bit3)<<3)

    if palidx != 0 {
      color := bus.vce.pal[0x100+pal+palidx]
      color.l = fg
      plot_dot(bus, int(x)+px, py, color) 
    }
  }
}

draw_sprite_strip :: proc(bus: ^Bus, vdc: ^VDC, sprites: []VRAM_Sprite, y: int) {
  #reverse for spr, i in sprites {
   	// size in tiles
   	tw, th := vram_sprite_size(spr)
   	spr_x, spr_y := cast(int)spr.x-32, cast(int)spr.y-64
    pal_offs := spr.flags.pal*16
    y_offs := y - spr_y
    
   	ty := y_offs/16
    sub_y := y_offs%16
    if spr.flags.yflip do sub_y = 15 - sub_y
    
   	for tx in 0..<tw {
     	i, j := tx, ty 
     	if spr.flags.xflip do i = tw - i - 1
     	if spr.flags.yflip do j = th - j - 1
      x := spr_x + cast(int)tx*16
      
      tile_addr := cast(int)(spr.tile<<5) + (i * 64) + (j * 128)
      
      draw_sprite_tile_strip(
        bus   = bus,
        vdc   = vdc,
        px    = x,
        py    = y,
        pal   = pal_offs,
        taddr = cast(uint)tile_addr&0x7FFF,
        sub_y = cast(uint)sub_y,
        flip  = spr.flags.xflip,
        fg    = spr.flags.foreground,
      )
    }
  }
}

vdc_sample_tile :: proc(bus: ^Bus, vdc: ^VDC, x, y: uint) -> Maybe(RGB333) {
  tmap_size := tmap_size_table[vdc.mwr.tmap_size]
  tx, sx := (x/8)%tmap_size.x, x%8
  ty, sy := (y/8)%tmap_size.y, y%8
  tile := vdc.vram.vram[cast(uint)tx+cast(uint)ty*tmap_size.x]
  tile_pal := cast(uint)(tile >> 12)*16
  tile_def := cast(uint)(tile << 4)&0x7FFF

  bit0 := (vdc.vram.vram[tile_def+sy+0]&(1<<(15-(sx+8)))) != 0
  bit1 := (vdc.vram.vram[tile_def+sy+0]&(1<<(15-(sx)))) != 0
  bit2 := (vdc.vram.vram[tile_def+sy+8]&(1<<(15-(sx+8)))) != 0
  bit3 := (vdc.vram.vram[tile_def+sy+8]&(1<<(15-(sx)))) != 0

  palidx := uint(bit0) | (uint(bit1)<<1) | (uint(bit2)<<2) | (uint(bit3)<<3)

  if palidx != 0 {
    return bus.vce.pal[tile_pal+palidx]
  }

  return nil
}

vdc_draw_tilemap_contents :: proc(bus: ^Bus, vdc: ^VDC, dest: []RGB333, dest_w, dest_h: uint) {
  tmap_size := tmap_size_table[vdc.mwr.tmap_size]
  for x in 0..<uint(tmap_size.x*8) {
    for y in 0..<uint(tmap_size.y*8) {
      if color, ok := vdc_sample_tile(bus, vdc, x, y).?; ok {
      dest[x+y*dest_w] = color
      }
    }
  }
}

vdc_draw_scanline :: proc(bus: ^Bus, vdc: ^VDC, y: int) {

	for i in 0..<int(vdc.out_width) {
    bus.screen[i+y*512] = bus.vce.pal[0]
  }
  
  if vdc.cr.spr_enable {
	  sprites: small_array.Small_Array(16, VRAM_Sprite)
	  
		for i in 0..<uint(64) {
	    sprite := vram_get_sprite(&vdc.vram, i)
	    w, h := vram_sprite_dims(sprite)
	
	    if y >= cast(int)sprite.y-64 && y < cast(int)sprite.y-64+cast(int)h {
	      if !small_array.append_elem(&sprites, sprite) {
	        // todo: spr overflow
	      }
	    }
	  }
	  
		draw_sprite_strip(bus, vdc, small_array.slice(&sprites), y)
	}

  
  if vdc.cr.bkg_enable {
    for x in 0..<vdc.out_width {
      if color, ok := vdc_sample_tile(bus, vdc, cast(uint)vdc.tx+x, cast(uint)vdc.ty).?; ok {
      	index := x+uint(y)*512
      	if !bus.screen[index].l {
      		bus.screen[index] = color
      	}
      }
    }
  }
}

vdc_cyc := 0

vdc_cycle :: proc(bus: ^Bus, vdc: ^VDC) {
	switch bus.vce.ctrl.freq {
	case .Mhz5:  vdc.out_width = 256
	case .Mhz7:  vdc.out_width = 340
	case .Mhz10: vdc.out_width = 512
	case:        vdc.out_width = 512
	}
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

        if vdc.dmactrl.vram_to_vram_int {
          vdc.status.vram_dma_end = true
          bus_irq(bus, .IRQ1)
        }
      }
    }

    if vdc.satb_dma_index < 0x100 {
     	index := (cast(int)vdc.dmaspr+vdc.satb_dma_index)&0x7FFF
    	vdc.vram.satb[vdc.satb_dma_index] = vdc.vram.vram[index]
      vdc.satb_dma_index += 1
      
      if vdc.satb_dma_index == 0x100 {
	      if vdc.dmactrl.vram_to_satb_int {
	        vdc.status.vram_to_satb_end = true
	        bus_irq(bus, .IRQ1)
	      }
      }
    }
    
    if vdc.x == 0 {
	    if vdc.rcr >= 64 && vdc.rcr <= 326 && vdc.y < 262 && !(vdc.y >= 245 && vdc.y <= 247) && vdc.cr.scanline_int && int(vdc.rcr)-64 == vdc.y {
	      log_instr_info("rcr!")
	      vdc.status.scanline_happen = true
	      bus_irq(bus, .IRQ1)
	    }
					
			if vdc.y == 245 && vdc.cr.vblank_int {
	      log_instr_info("vblank!")
	      vdc.status.vblank_happen = true
	     	vdc.satb_dma_index = 0
	      bus_irq(bus, .IRQ1)
	    }
    }
    vdc.x += 1
    if vdc.x == 65 {
	    if vdc.y < 224 {
	      vdc_draw_scanline(bus, vdc, vdc.y)
	    }
    }
    if vdc.x >= 65 && vdc.x <= 65+cast(int)vdc.out_width {
	    vdc.tx += 1
    }
    if vdc.x == 341 {
      vdc.x = 0
      vdc.tx = vdc.scroll_x
 
	    vdc.y += 1
			vdc.ty += 1

	    if vdc.y == 262 {
	      vdc.y = 0
	      vdc.ty = vdc.scroll_y
	      bus.vblank_occured = true
	    }
    }
  }
}

vdc_write_reg :: proc(vdc: ^VDC, reg: VDC_Reg, val: u16) {
  log_instr_info("vdc write reg: %v %04X", reg, val)

  switch reg {
  case .Mawr: vdc.mawr = val&0x7FFF
  case .Marr: vdc.marr = val&0x7FFF
  case .Vrw: panic("vrw write is special")
  case .Unused0, .Unused1:
  case .Cr: vdc.cr = cast(VDC_Cr)val
  case .Rcr: vdc.rcr = val&0x3FF
  case .Scrollx: vdc.scroll_x = val&0x3FF
  case .Scrolly: vdc.scroll_y = val&0x1FF; vdc.ty = vdc.scroll_y
  case .Mwr:  vdc.mwr = cast(VDC_Mwr)val
  case .Hsync, .Hdisp, .Vsync, .Vdisp, .Vend:
    log.warnf("todo: vdc sync register write")
  case .DMActrl: vdc.dmactrl = cast(VDC_DMA_Ctrl)val
  case .DMAsrc: vdc.dmasrc = val&0x7FFF
  case .DMAdst: vdc.dmadst = val&0x7FFF
  case .DMAlen: vdc.dmalen = val
  case .DMAspr: vdc.dmaspr = val&0x7FFF; vdc.satb_dma_next_vblank = true
  }
}

vdc_read_reg :: proc(vdc: ^VDC, reg: VDC_Reg) -> (ret: u16) {
  switch reg {
  case .Mawr: ret = vdc.mawr
  case .Marr: ret = vdc.marr
  case .Vrw:  ret = vdc.vram.vram[vdc.marr]
  case .Unused0, .Unused1:
  case .Cr:      ret = cast(u16)vdc.cr
  case .Rcr:     ret = vdc.rcr
  case .Scrollx: ret = cast(u16)vdc.scroll_x
  case .Scrolly: ret = cast(u16)vdc.scroll_y
  case .Mwr:     ret = cast(u16)vdc.mwr
  case .Hsync, .Hdisp, .Vsync, .Vdisp, .Vend:
    log.warnf("todo: vdc sync register read")
  case .DMActrl: ret = cast(u16)vdc.dmactrl
  case .DMAsrc:  ret = vdc.dmasrc
  case .DMAdst:  ret = vdc.dmadst
  case .DMAlen:  ret = vdc.dmalen
  case .DMAspr:  ret = vdc.dmaspr
  }
    
  log_instr_info("vdc read reg: %v %04X", reg, ret)
  return
}

vdc_write :: proc(bus: ^Bus, vdc: ^VDC, addr: VDC_Addrs, val: u8) {
	switch addr {
  case .Ctrl: vdc.reg_selected = cast(VDC_Reg)(val & 0x1F); if val > 19 do log.warnf("invalid reg selected %d", val)
  case .Unknown1:
  case .Data_lo:
    if vdc.reg_selected == .Vrw {
      vdc.vram_buf = cast(u16)val
    } else {
      vdc_write_reg(vdc, vdc.reg_selected, (vdc_read_reg(vdc, vdc.reg_selected)&0xFF00)|cast(u16)val)
    }
  case .Data_hi:
    if vdc.reg_selected == .Vrw {
      vdc.vram.vram[vdc.mawr] = vdc.vram_buf|(cast(u16)val<<8)
      vdc.vram_buf = 0
      vdc.mawr += vdc_rw_increment(vdc)
      vdc.mawr &= 0x7FFF
    } else {
      vdc_write_reg(vdc, vdc.reg_selected, (vdc_read_reg(vdc, vdc.reg_selected)&0xFF)|(cast(u16)val << 8))
      if vdc.reg_selected == .DMAlen {
        vdc.dma_state = .Read_Src
      }
    }
  }
}

vdc_read :: proc(bus: ^Bus, vdc: ^VDC, addr: VDC_Addrs) -> (ret: u8) {
  switch addr {
  case .Ctrl: ret = cast(u8)vdc.status; vdc.status = cast(VDC_Status)0
  case .Unknown1:
  case .Data_lo: ret = cast(u8)(vdc_read_reg(vdc, vdc.reg_selected)&0xFF)
  case .Data_hi:
    ret = cast(u8)(vdc_read_reg(vdc, vdc.reg_selected)>>8)
    if vdc.reg_selected == .Vrw {
      vdc.marr += vdc_rw_increment(vdc)
      vdc.marr &= 0x7FFF
    }
  }

  return
}
