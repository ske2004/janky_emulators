#include <stdint.h>
#include "ppu.c"

enum vector
{
    VEC_NMI,
    VEC_RESET,
    VEC_IRQ
};

struct nrom
{
    struct ricoh_decoder decoder;
    struct ricoh_state cpu;
    struct ppu ppu;

    uint8_t prgsize;
    uint8_t chrsize;
    uint8_t memory[1<<16];
};

static void _nrom_mem_write(void *mem, uint16_t addr, uint8_t data)
{
    struct nrom *nrom = mem;

    switch (addr)
    {
        case 0x2000: ppu_write(&nrom->ppu, PPUIO_CTRL, data); break;
        case 0x2001: ppu_write(&nrom->ppu, PPUIO_MASK, data); break;
        case 0x2003: ppu_write(&nrom->ppu, PPUIO_OAMADDR, data); break;
        case 0x2004: ppu_write(&nrom->ppu, PPUIO_OAMDATA, data); break;
        case 0x2005: ppu_write(&nrom->ppu, PPUIO_SCROLL, data); break;
        case 0x2006: ppu_write(&nrom->ppu, PPUIO_ADDR, data); break;
        case 0x2007: ppu_write(&nrom->ppu, PPUIO_DATA, data); break;
        case 0x4014: ppu_write(&nrom->ppu, PPUIO_OAMDMA, data); break;
        default:
            if (nrom->prgsize == 1 && addr >= 0xC000) addr -= 0x4000;
            nrom->memory[addr] = data;
            break;
    }
}

static uint8_t _nrom_mem_read(void *mem, uint16_t addr)
{
    struct nrom *nrom = mem;
 
    switch (addr)
    {
        case 0x2002: return ppu_read(&nrom->ppu, PPUIO_STATUS);
        case 0x2004: return ppu_read(&nrom->ppu, PPUIO_OAMDATA);
        case 0x2007: return ppu_read(&nrom->ppu, PPUIO_DATA);
        default: 
            if (nrom->prgsize == 1 && addr >= 0xC000) addr -= 0x4000;
            return nrom->memory[addr];
    }

    return 0;
}

uint16_t nrom_get_vector(struct nrom *nrom, enum vector vec)
{
    switch (vec)
    {
        case VEC_NMI: return   _nrom_mem_read(nrom, 0xFFFA) | (_nrom_mem_read(nrom, 0xFFFB) << 8);
        case VEC_RESET: return _nrom_mem_read(nrom, 0xFFFC) | (_nrom_mem_read(nrom, 0xFFFD) << 8);
        case VEC_IRQ: return   _nrom_mem_read(nrom, 0xFFFE) | (_nrom_mem_read(nrom, 0xFFFF) << 8);
    }
}

uint8_t nrom_load(uint8_t *ines, struct nrom *out)
{
    if (!(ines[0] == 'N' && ines[1] == 'E' && ines[2] == 'S' && ines[3] == 0x1A))
    {
        return 1;
    }

    *out = (struct nrom){ 0 };
    out->prgsize = ines[4];
    out->chrsize = ines[5];

    uint32_t prg_size = out->prgsize*(1<<14);

    if (prg_size > 0x8000) 
    {
        return 2;
    }

    out->decoder = make_ricoh_decoder();
    
    memcpy(out->memory+0x8000, ines+16, prg_size);

    out->cpu = (struct ricoh_state){ 0 };
    out->cpu.pc = nrom_get_vector(out, VEC_RESET);
    out->cpu.flags = 0x24;
    out->cpu.sp = 0xFD;
    out->cpu.cycles = 7;

    uint32_t chr_size = out->chrsize*(1<<13);

    if (chr_size > 0x2000)
    {
        return 3;
    }
    
    out->ppu = ppu_mk();
    ppu_write_chr(&out->ppu, ines+16+prg_size, chr_size);

    return 0;
}

struct ricoh_mem_interface nrom_get_memory_interface(struct nrom *nrom)
{
    return (struct ricoh_mem_interface){
        nrom,
        _nrom_mem_read,
        _nrom_mem_write,
    };
}

struct nrom_frame_result
{
    uint8_t screen[240*256];
};

struct nrom_frame_result nrom_frame(struct nrom *nrom)
{
    struct ricoh_mem_interface mem = nrom_get_memory_interface(nrom);

    uint32_t start = nrom->cpu.cycles;

    while ((nrom->cpu.cycles-start) < 50000)
    {
        uint16_t start = nrom->cpu.pc;
        struct instr_decoded decoded = ricoh_decode_instr(&nrom->decoder, &mem, nrom->cpu.pc);
        
        // char buf[1024] = { 0 };
        // ricoh_format_decoded_instr(buf, decoded);

        // printf("%04X  ", nrom->cpu.pc);
        // switch (decoded.size)
        // {
        //     case 1: printf("%02X        ", mem.get(mem.instance, start)); break;
        //     case 2: printf("%02X %02X     ", mem.get(mem.instance, start), mem.get(mem.instance, start+1)); break;
        //     case 3: printf("%02X %02X %02X  ", mem.get(mem.instance, start), mem.get(mem.instance, start+1), mem.get(mem.instance, start+2)); break;
        // }

        // printf("%s", buf);
        // for (size_t i = 0; i < 32-strlen(buf); i++)
        // {
        //     printf(" ");
        // }
        // printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%d\n", nrom->cpu.a,nrom-> cpu.x, nrom->cpu.y, nrom->cpu.flags, nrom->cpu.sp, nrom->cpu.cycles);

        ricoh_run_instr(&nrom->cpu, decoded, &mem);
    }

    ppu_vblank(&nrom->ppu);
    if (ppu_nmi_enabled(&nrom->ppu))
    {
        ricoh_do_interrupt(&nrom->cpu, &mem, nrom_get_vector(nrom, VEC_NMI));
    }
        
    struct nrom_frame_result result = { 0 };
    ppu_get_buf(&nrom->ppu, result.screen);

    return result;
}