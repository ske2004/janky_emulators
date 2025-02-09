#include "ricoh.c"
#include <assert.h>

enum ppu_ir {
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

enum ppu_io {
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
    uint8_t vram[1<<14];
    uint8_t extlatch;
    uint8_t regs[PPUIR_COUNT];
};

struct ppu ppu_mk()
{
    return (struct ppu){ 0 };
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

void ppu_get_buf(struct ppu *ppu, struct ricoh_mem_interface *mem, uint8_t screen[240*256])
{
    for (int x = 0; x < 256/8; x++)
    {
        for (int y = 0; y < 240/8; y++)
        {
            uint16_t idx = x+y*(256/8);
            uint16_t tile = ppu->vram[0x2000+idx];

            uint8_t octx =  (x/4);
            uint8_t octy =  (y/4);
            uint8_t quadx = (x/2)&1;
            uint8_t quady = (y/2)&1;
            uint8_t palidx = (ppu->vram[0x23C0+octx+octy*8]>>((quadx|(1<<quady))<<1))&3;

            if (ppu->regs[PPUIR_CTRL] & (1<<4)) tile += 0x100;

            for (int tx = 0; tx < 8; tx++)
            {
                for (int ty = 0; ty < 8; ty++)
                {
                    uint8_t lo = (ppu->vram[(uint16_t)tile*16+ty]>>(7-tx))&1;
                    uint8_t hi = (ppu->vram[(uint16_t)tile*16+8+ty]>>(7-tx))&1;
                    uint8_t palcoloridx = lo | (hi << 1);
                    uint8_t palcolor = ppu->vram[0x3F00+palidx*4+palcoloridx];
                    if (palcoloridx == 0) {
                        palcolor = ppu->vram[0x3F10];
                    }


                    int _x = x*8+tx;
                    int _y = y*8+ty;
                    screen[_x+_y*256] = palcolor;
                }
            }
        }
    }
    
    for (int o = 0; o < 256/4; o++)
    {
        uint8_t y     = mem->get(mem->instance, o*4+ppu->regs[PPUIR_OAMDMA]*256+0)-1;
        uint16_t tile = mem->get(mem->instance, o*4+ppu->regs[PPUIR_OAMDMA]*256+1);
        uint8_t pal   = mem->get(mem->instance, o*4+ppu->regs[PPUIR_OAMDMA]*256+2);
        uint8_t x     = mem->get(mem->instance, o*4+ppu->regs[PPUIR_OAMDMA]*256+3);
        
        printf("%d %d\n", x, y);

        if (ppu->regs[PPUIR_CTRL] & (1<<3)) tile += 0x100;

        uint8_t palidx = pal&3;

        for (int tx = 0; tx < 8; tx++)
        {
            for (int ty = 0; ty < 8; ty++)
            {
                uint8_t lo = (ppu->vram[(uint16_t)tile*16+ty]>>(7-tx))&1;
                uint8_t hi = (ppu->vram[(uint16_t)tile*16+8+ty]>>(7-tx))&1;
                uint8_t palcoloridx = lo | (hi << 1);
                uint8_t palcolor = ppu->vram[0x3F10+palidx*4+palcoloridx];
                
                if (palcoloridx == 0) {
                    continue;
                }

                int _x = x+tx;
                int _y = y+ty;
                screen[_x+_y*256] = palcolor;
            }
        }
    }
}