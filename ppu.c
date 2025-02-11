#include "ricoh.c"
#include <assert.h>

enum ppu_ir
{
    PPUIR_CTRL,
    PPUIR_MASK,
    PPUIR_STATUS,
    PPUIR_OAMADDR,
    PPUIR_OAMDATA,
    PPUIR_SCROLLXADDRHI,
    PPUIR_SCROLLYADDRLO,
    PPUIR_DATA,
    PPUIR_OAMDMA,
    PPUIR_COUNT
};

enum ppu_io
{
    PPUIO_CTRL,
    PPUIO_MASK,
    PPUIO_STATUS,
    PPUIO_OAMADDR,
    PPUIO_OAMDATA,
    PPUIO_SCROLL,
    PPUIO_ADDR,
    PPUIO_DATA,
    PPUIO_OAMDMA,
    PPUIO_COUNT
};

struct ppu
{
    uint8_t oam[256];
    uint8_t vram[1<<14];
    uint8_t extlatch;
    uint8_t regs[PPUIR_COUNT];
    uint16_t beam;
    uint16_t scanline;
    uint32_t ticks;
    uint8_t screen[256*240];
};

struct ppu ppu_mk()
{
    struct ppu ppu = { 0 };
    return ppu;
}

void ppu_write_chr(struct ppu *ppu, uint8_t *chr, uint32_t chr_size)
{
    assert(chr_size <= 0x2000);

    memcpy(ppu->vram, chr, chr_size);
}

uint16_t ppu_get_addr(struct ppu *ppu)
{
    return (((uint16_t)ppu->regs[PPUIR_SCROLLXADDRHI] << 8) |
            ((uint16_t)ppu->regs[PPUIR_SCROLLYADDRLO])) & 0x3FFF;
}

void ppu_set_addr(struct ppu *ppu, uint16_t addr)
{
    ppu->regs[PPUIR_SCROLLYADDRLO] = addr & 0xFF;
    ppu->regs[PPUIR_SCROLLXADDRHI] = (addr >> 8) & 0xFF;
}

void ppu_write(struct ppu *ppu, enum ppu_io io, uint8_t data)
{
    switch (io)
    {
        case PPUIO_CTRL: ppu->regs[PPUIR_CTRL] = data; break;
        case PPUIO_MASK: ppu->regs[PPUIR_MASK] = data; break;
        case PPUIO_OAMADDR: ppu->regs[PPUIR_OAMADDR] = data; break;
        case PPUIO_OAMDATA: ppu->regs[PPUIR_OAMDATA] = data; break;
        case PPUIO_SCROLL: 
        case PPUIO_ADDR: 
            if (ppu->extlatch) 
            { ppu->extlatch = 0; ppu->regs[PPUIR_SCROLLYADDRLO] = data; }
            else
            { ppu->extlatch = 1; ppu->regs[PPUIR_SCROLLXADDRHI] = data;  }
            break;
        case PPUIO_DATA:
            // printf("Write! %x -> %x\n", data, ppu_get_addr(ppu));
            ppu->vram[ppu_get_addr(ppu)] = data;
            if (ppu->regs[PPUIR_CTRL] & (1 << 2)) {
                ppu_set_addr(ppu, ppu_get_addr(ppu)+32);
            } else {
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
    ppu->regs[PPUIR_STATUS] = ppu->regs[PPUIR_STATUS] | (1<<7);
}

uint8_t ppu_vram_read(struct ppu *ppu, uint16_t addr)
{
    return ppu->vram[ppu_get_addr(ppu)];
}

void ppu_vram_write(struct ppu *ppu, uint16_t addr, uint8_t val)
{
    ppu->vram[ppu_get_addr(ppu)] = val;
}

uint8_t ppu_read(struct ppu *ppu, enum ppu_io io)
{
    switch (io)
    {
        case PPUIO_STATUS: 
            {
                uint8_t result = ppu->regs[PPUIR_STATUS];
                ppu->regs[PPUIR_STATUS] = ppu->regs[PPUIR_STATUS]<<1>>1;
                return result;
            }
            break;
        case PPUIO_OAMDATA: return ppu->regs[PPUIR_OAMDATA]; break;
        case PPUIO_DATA:
            {
                uint8_t value = ppu->regs[PPUIR_DATA];
                ppu->regs[PPUIR_DATA] = ppu_vram_read(ppu, ppu_get_addr(ppu));
                if (ppu->regs[PPUIR_CTRL] & (1 << 2)) {
                    ppu_set_addr(ppu, ppu_get_addr(ppu)+32);
                } else {
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

uint8_t ppu_get_pixel(struct ppu *ppu, int x, int y)
{
    uint8_t pixel = 0;
    bool opaque = false;

    // Get tile pixel
    {
        int tile_x = x/8;
        int tile_y = y/8;

        uint16_t idx = tile_x+tile_y*(256/8);
        uint16_t tile = ppu->vram[0x2000+idx];

        uint8_t octx =  (tile_x/4);
        uint8_t octy =  (tile_y/4);
        uint8_t quadx = (tile_x/2)&1;
        uint8_t quady = (tile_y/2)&1;
        uint8_t palidx = (ppu->vram[0x23C0+octx+octy*8]>>((quadx|(1<<quady))<<1))&3;

        if (ppu->regs[PPUIR_CTRL] & (1<<4)) tile += 0x100;

        int tx = x%8;
        int ty = y%8;

        uint8_t lo = (ppu->vram[(uint16_t)tile*16+ty]>>(7-tx))&1;
        uint8_t hi = (ppu->vram[(uint16_t)tile*16+8+ty]>>(7-tx))&1;
        uint8_t palcoloridx = lo | (hi << 1);
        uint8_t palcolor = ppu->vram[0x3F00+palidx*4+palcoloridx];
        if (palcoloridx == 0) {
            palcolor = ppu->vram[0x3F10];
        }

        opaque = palcoloridx != 0;

        pixel = palcolor;
    }


    for (int o = 0; o < 64; o++)
    {
        uint8_t oy    = ppu->oam[o*4+0];
        uint16_t tile = ppu->oam[o*4+1];
        uint8_t pal   = ppu->oam[o*4+2];
        uint8_t ox    = ppu->oam[o*4+3];

        if (oy == 0) continue;

        if (x-ox >= 8 || x-ox < 0 || y-oy >= 8 || y-oy < 0)
        {
            continue;
        }

        int tx = x-ox;
        int ty = y-oy;

        if (ppu->regs[PPUIR_CTRL] & (1<<3)) tile += 0x100;
        uint8_t palidx = pal&3;

        uint8_t lo = (ppu->vram[tile*16+ty]>>(7-tx))&1;
        uint8_t hi = (ppu->vram[tile*16+8+ty]>>(7-tx))&1;
        uint8_t palcoloridx = lo | (hi << 1);
        uint8_t palcolor = ppu->vram[0x3F10+palidx*4+palcoloridx];
     
        if (palcoloridx == 0) 
        {
            continue;
        }

        if (opaque && o == 0)
        {
            ppu_write(ppu, PPUIO_STATUS, ppu_read(ppu, PPUIO_STATUS)|(1<<6));
        }
        
        pixel = palcolor;
    }

    return pixel;
}

bool ppu_cycle(struct ppu *ppu, struct ricoh_mem_interface *mem)
{
    bool nmi_occured = false;

    ppu->ticks += 1;

    if (ppu->beam > 340)
    {
        ppu->scanline += 1;
        ppu->beam = 0;
    }
    
    if (ppu->scanline < 240) 
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
        if (ppu->beam == 0)
        {
            ppu_vblank(ppu);
            nmi_occured = true;
        }
    }
    else if (ppu->scanline > 240 && ppu->scanline <= 260)
    {
        // vblank
    }
    else
    {
        ppu->scanline = 0;
        ppu->beam = -1;
        ppu_write(ppu, PPUIO_STATUS, ppu_read(ppu, PPUIO_STATUS)&~(1<<6));
    }

    ppu->beam += 1;

    return nmi_occured;
}
