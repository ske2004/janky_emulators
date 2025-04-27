#include "neske.h"
#include <assert.h>
#include <string.h>

uint8_t *ppu_vram_get_ptr(struct ppu *ppu, uint16_t addr)
{
    if (addr >= 0x0000 && addr < 0x2000)
    {
        return ppu->pins.chr + addr;
    }

    if (addr >= 0x2000 && addr < 0x3000)
    {
        if (ppu->pins.mirroring_mode == PPUMIR_HOR)
        {
            if (addr >= 0x2400 && addr < 0x2800)
            {
                addr -= 0x400;
            }
            else if (addr >= 0x2800 && addr < 0x2C00)
            {
                addr -= 0x400;
            }
            else if (addr >= 0x2C00 && addr < 0x3000)
            {
                addr -= 0x800;
            }
        }
        else
        {
            if (addr >= 0x2800 && addr < 0x3000)
            {
                addr -= 0x800;
            }
        }

        return ppu->vram + (addr-0x2000);
    }

    if (addr >= 0x3F00 && addr <= 0x4000)
    {
        addr = addr % 0x20;

        // Pallete first index mirroring
        if ((addr % 4) == 0)
        {
            addr = addr & 0xF;
        }

        return ppu->pallete + addr;
    }

    return NULL;
}

uint8_t ppu_vram_read(struct ppu *ppu, uint16_t addr)
{
    uint8_t *ptr = ppu_vram_get_ptr(ppu, addr);
    if (ptr == NULL)
    {
        return 0;
    }

    return ptr[0];
}

void ppu_vram_write(struct ppu *ppu, uint16_t addr, uint8_t val)
{
    if (addr >= 0x0000 && addr < 0x2000)
    {
        return;
    }

    uint8_t *ptr = ppu_vram_get_ptr(ppu, addr);
    if (ptr == NULL)
    {
        return;
    }

    ptr[0] = val;
}

struct ppu ppu_mk()
{
    struct ppu ppu = { 0 };
    return ppu;
}

uint16_t ppu_get_addr(struct ppu *ppu)
{
    return ppu->v & 0x3FFF;
}

void ppu_set_addr(struct ppu *ppu, uint16_t addr)
{
    ppu->v = addr & 0x3FFF;
}

void ppu_write(struct ppu *ppu, enum ppu_io io, uint8_t data)
{
    switch (io)
    {
        case PPUIO_CTRL:
            ppu->regs[PPUIR_CTRL] = data;
            ppu->t &= ~((1<<10)|(1<<11));
            ppu->t |= (data&0b11)<<10;
            break;
        case PPUIO_MASK: ppu->regs[PPUIR_MASK] = data; break;
        case PPUIO_OAMADDR: ppu->regs[PPUIR_OAMADDR] = data; break;
        case PPUIO_OAMDATA: ppu->regs[PPUIR_OAMDATA] = data; break;
        case PPUIO_ADDR: 
            if (ppu->w == 0) 
            {
                ppu->w = 1;
                ppu->t &= ~(((1<<7)-1)<<8);
                ppu->t |= (data&((1<<6)-1))<<8;
            }
            else
            {
                ppu->w = 0;
                ppu->t &= ~((1<<8)-1);
                ppu->t |= data;
                ppu->v = ppu->t;
            }
            break;
        case PPUIO_SCROLL: 
            if (ppu->w == 0) 
            {
                ppu->w = 1;
                ppu->t &= ~((1<<5)-1);
                ppu->t |= (data>>3)&((1<<5)-1);
                ppu->x = data&((1<<3)-1);
            }
            else
            {
                ppu->w = 0;
                ppu->t &= ~(((1<<5)-1)<<5);
                ppu->t |= ((data>>3)&((1<<5)-1))<<5;
                ppu->t &= ~(((1<<3)-1)<<12);
                ppu->t |= ((data)&((1<<3)-1))<<12;
            }
            break;
        case PPUIO_DATA:
            ppu_vram_write(ppu, ppu_get_addr(ppu), data);
            if (ppu->regs[PPUIR_CTRL] & (1 << 2))
            {
                ppu_set_addr(ppu, ppu_get_addr(ppu)+32);
            }
            else
            {
                ppu_set_addr(ppu, ppu_get_addr(ppu)+1);
            }
            break;
        case PPUIO_OAMDMA: 
            ppu->regs[PPUIR_OAMDMA] = data;
            break;
        default: return;
    }
}

void ppu_vblank(struct ppu *ppu)
{
    ppu->regs[PPUIR_STATUS] |= 1<<7;
}

uint8_t ppu_read(struct ppu *ppu, enum ppu_io io)
{
    switch (io)
    {
        case PPUIO_STATUS: 
            {
                uint8_t result = ppu->regs[PPUIR_STATUS];
                ppu->regs[PPUIR_STATUS] = (ppu->regs[PPUIR_STATUS]<<1)>>1;
                ppu->w = 0;
                return result;
            }
            break;
        case PPUIO_OAMDATA: return ppu->regs[PPUIR_OAMDATA]; break;
        case PPUIO_DATA:
            {
                uint8_t value = ppu->regs[PPUIR_DATA];
                ppu->regs[PPUIR_DATA] = ppu_vram_read(ppu, ppu_get_addr(ppu));
                if (ppu->regs[PPUIR_CTRL] & (1 << 2))
                {
                    ppu_set_addr(ppu, ppu_get_addr(ppu)+32);
                }
                else
                {
                    ppu_set_addr(ppu, ppu_get_addr(ppu)+1);
                }
                return value;
            }
            break;
        default: return 0;
    }

    return 0;
}

bool ppu_nmi_enabled(struct ppu *ppu)
{
    return (ppu->regs[PPUIR_CTRL] & 0x80) > 0;
}

void ppu_write_oam(struct ppu *ppu, uint8_t *oamsrc)
{
    memcpy(ppu->oam, oamsrc, 256);
}

struct ppu_nametable_result
{
    uint8_t tile;
    uint8_t palidx;
};

struct ppu_nametable_result ppu_read_nametable(struct ppu *ppu, uint8_t x, uint8_t y)
{
    x %= 64;
    y %= 60;

    if (ppu->pins.mirroring_mode == PPUMIR_HOR)
    {
        x %= 32;
    }
    else
    {
        y %= 30;
    }

    uint16_t addr = 0x2000;

    if (x >= 32) 
    {
        addr += 0x400;
        x -= 32;
    }
    if (y >= 30) 
    {
        addr += 0x800;
        y -= 30;
    }

    uint8_t octx =  (x>>2);   // 0 0
    uint8_t octy =  (y>>2);   // 1 1
    uint8_t quadx = (x>>1)&1; // 0 1
    uint8_t quady = (y>>1)&1; // 0 0
    uint8_t q = (quadx|(quady<<1))<<1; // right
    uint8_t palidx = (ppu_vram_read(ppu, addr+0x3C0+octx+octy*8)>>q)&3;

    return (struct ppu_nametable_result) {
        ppu_vram_read(ppu, addr + x + y * 32),
        palidx
    };
}

void ppu_get_scroll(struct ppu *ppu, uint16_t *sx, uint16_t *sy)
{
    *sx = (((ppu->t&((1<<5)-1))<<3) |   // XXXXX
          (ppu->x&((1<<3)-1))) +        // Fine xxx
          ((ppu->t&(1<<10)) ? 256 : 0); // Nametable select

    *sy = (((ppu->t>>5)&((1<<5)-1))<<3) | // YYYYY
          ((ppu->t>>12)&((1<<3)-1))       // Fine yyy
          ;

    // temp hack for binary land to work
    if (*sy >= 240) *sy += 240-16;
    *sy += ((ppu->t&(1<<11)) ? 240 : 0);
}

uint8_t ppu_get_pixel(struct ppu *ppu, int x, int y)
{
    uint8_t pixel = 15;
    bool opaque = false;

    // Get tile pixel
    if (ppu->regs[PPUIR_MASK]&(1<<3))
    {
        uint16_t scroll_x = 0, scroll_y = 0;
        ppu_get_scroll(ppu, &scroll_x, &scroll_y);

        int sx = (x + scroll_x);
        int sy = (y + scroll_y);

        int tile_x = sx/8;
        int tile_y = sy/8;

        struct ppu_nametable_result ntr = ppu_read_nametable(ppu, tile_x, tile_y);

        uint16_t tile = ntr.tile;
        uint8_t palidx = ntr.palidx;

        if (ppu->regs[PPUIR_CTRL] & (1<<4)) tile += 0x100;

        int tx = sx%8;
        int ty = sy%8;

        uint8_t lo = (ppu_vram_read(ppu, (uint16_t)tile*16+ty)>>(7-tx))&1;
        uint8_t hi = (ppu_vram_read(ppu, (uint16_t)tile*16+8+ty)>>(7-tx))&1;
        uint8_t palcoloridx = lo | (hi << 1);
        uint8_t palcolor = ppu_vram_read(ppu, 0x3F00+palidx*4+palcoloridx);
        if (palcoloridx == 0)
        {
            palcolor = ppu_vram_read(ppu, 0x3F00);
        }

        opaque = palcoloridx != 0;
        pixel = palcolor;
    }

    if (ppu->regs[PPUIR_MASK]&(1<<4))
    {
        if (ppu->regs[PPUIR_CTRL]&(1<<5))
            for (int o = 0; o < ppu->preload_objects_count; o++)
            {
                struct ppu_object obj = ppu->preload_objects[o];
               
                uint16_t tile1 = (obj.tile&~1)+((obj.tile&1)*0x100);
                uint16_t tile2 = tile1+1;

                if (obj.y == 0) continue;

                if (x-obj.x >= 8 || x-obj.x < 0 || y-obj.y >= 16 || y-obj.y < 0)
                {
                    continue;
                }

                int tx = x-obj.x;
                if (obj.attr & (1<<6)) tx = 8-tx-1;
                int ty = y-obj.y;
                if (obj.attr & (1<<7))
                {
                    ty = 16-ty-1;
                }
                
                uint16_t tile = ty >= 8 ? tile2 : tile1;
                uint8_t palidx = obj.attr&3;
                ty %= 8;

                uint8_t lo = (ppu_vram_read(ppu, tile*16+ty)>>(7-tx))&1;
                uint8_t hi = (ppu_vram_read(ppu, tile*16+8+ty)>>(7-tx))&1;
                uint8_t palcoloridx = lo | (hi << 1);
                uint8_t palcolor = ppu_vram_read(ppu, 0x3F10+palidx*4+palcoloridx);
             
                if (palcoloridx == 0) 
                {
                    continue;
                }

                if (opaque && o == 0 && ppu->preload_objects_sprite_0)
                {
                    ppu->regs[PPUIO_STATUS] = ppu->regs[PPUIO_STATUS]|(1<<6);
                }
                
                if (!(obj.attr & (1<<5) && opaque))
                {
                    pixel = palcolor;
                    break;
                }
            }
        else
            for (int o = 0; o < ppu->preload_objects_count; o++)
            {
                struct ppu_object obj = ppu->preload_objects[o];

                uint16_t tile = obj.tile;

                if (obj.y == 0) continue;

                if (x-obj.x >= 8 || x-obj.x < 0 || y-obj.y >= 8 || y-obj.y < 0)
                {
                    continue;
                }

                int tx = x-obj.x;
                if (obj.attr & (1<<6)) tx = 8-tx-1;
                int ty = y-obj.y;
                if (obj.attr & (1<<7)) ty = 8-ty-1;

                if (ppu->regs[PPUIR_CTRL] & (1<<3)) tile += 0x100;
                uint8_t palidx = obj.attr&3;

                uint8_t lo = (ppu_vram_read(ppu, tile*16+ty)>>(7-tx))&1;
                uint8_t hi = (ppu_vram_read(ppu, tile*16+8+ty)>>(7-tx))&1;
                uint8_t palcoloridx = lo | (hi << 1);
                uint8_t palcolor = ppu_vram_read(ppu, 0x3F10+palidx*4+palcoloridx);
             
                if (palcoloridx == 0) 
                {
                    continue;
                }

                if (opaque && o == 0 && ppu->preload_objects_sprite_0)
                {
                    ppu->regs[PPUIO_STATUS] = ppu->regs[PPUIO_STATUS]|(1<<6);
                }
                
                if (!(obj.attr & (1<<5) && opaque))
                {
                    pixel = palcolor;
                    break;
                }
            }
    }

    return pixel;
}

bool ppu_cycle(struct ppu *ppu, struct ricoh_mem_interface *mem)
{
    bool nmi_occured = false;

    ppu->cycles += 1;

    if (ppu->scanline == -1 && ppu->beam == 0)
    {
        // I didn't look into this properly but I assume you need to reset the sprite overflow flag
        ppu->regs[PPUIR_STATUS] &= ~(1<<5);
        ppu->regs[PPUIO_STATUS] &= ~(1<<7);
    }

    if (ppu->beam > 340)
    {
        if (ppu->scanline == 240) {
            ppu_vblank(ppu);
            nmi_occured = true;
        }
        ppu->scanline += 1;
        ppu->beam = 0;
        ppu->preload_objects_count = 0;
        ppu->preload_objects_sprite_0 = 0;

        for (int o = 0; o < 64; o++)
        {
            struct ppu_object obj = ppu->oam[o];
            // @TODO: Maybe not here
            obj.y += 1;

            int height = ppu->regs[PPUIR_CTRL]&(1<<5) ? 16 : 8;

            if (ppu->scanline >= obj.y && ppu->scanline < obj.y+height)
            {
                if (ppu->preload_objects_count == 8) 
                {
                    // Sprite overflow, but this isn't a proper way to handle it
                    // there's a bug in the original hardware, I cba to implement it right now -.-
                    ppu->regs[PPUIR_STATUS] |= 1<<5;
                    break;
                }

                if (o == 0)
                    ppu->preload_objects_sprite_0 = 1;

                ppu->preload_objects[ppu->preload_objects_count++] = obj;
            }
        }
    }
 
    if (ppu->scanline == -1)   
    {
        // nothing
    }
    else if (ppu->scanline < 240) 
    {
        if (ppu->beam < 256)
        {
            int x = ppu->beam;
            int y = ppu->scanline;

            uint8_t pixel = ppu_get_pixel(ppu, x, y);

            ppu->screen[x+y*256] = pixel;
        }
    }
    else if (ppu->scanline == 240)
    {
        // idle
    }
    else if (ppu->scanline > 240 && ppu->scanline <= 260)
    {
        // vblank
    }
    else
    {
        ppu->scanline = -1;
        ppu->beam = -1;
        ppu->regs[PPUIO_STATUS] = ppu->regs[PPUIO_STATUS]&~(1<<6);
    }

    ppu->beam += 1;

    return nmi_occured;
}
