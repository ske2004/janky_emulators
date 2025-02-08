#include "ricoh.c"

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
        case PPUIO_DATA: ppu->regs[PPUIR_DATA] = data; break;
        case PPUIO_OAMDMA: ppu->regs[PPUIR_OAMDMA] = data; break;
        default: return;
    }
}

uint16_t ppu_get_addr(struct ppu *ppu)
{
    return (((uint16_t)ppu->regs[PPUIR_SCROLLXADDRHI] << 8) |
            ((uint16_t)ppu->regs[PPUIR_SCROLLYADDRLO])) & 0x3FFF;
}

uint8_t ppu_read(struct ppu *ppu, enum ppu_io io)
{
    switch (io)
    {
        case PPUIO_STATUS: return ppu->regs[PPUIR_STATUS]; break;
        case PPUIO_OAMDATA: return ppu->regs[PPUIR_OAMDATA]; break;
        case PPUIO_DATA: return ppu->regs[PPUIR_DATA]; break;
        default: return 0;
    }

    return 0;
}
