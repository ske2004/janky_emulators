#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "neske.h"

static void _nrom_mem_write(void *mem, uint16_t addr, uint8_t data)
{
    struct nrom *nrom = mem;

    if (addr >= 0x2000 && addr < 0x4000)
    {
        addr = 0x2000 + ((addr-0x2000)%8);
    }

    switch (addr)
    {
        case 0x2000: ppu_write(&nrom->ppu, PPUIO_CTRL, data); break;
        case 0x2001: ppu_write(&nrom->ppu, PPUIO_MASK, data); break;
        case 0x2003: ppu_write(&nrom->ppu, PPUIO_OAMADDR, data); break;
        case 0x2004: ppu_write(&nrom->ppu, PPUIO_OAMDATA, data); break;
        case 0x2005: ppu_write(&nrom->ppu, PPUIO_SCROLL, data); break;
        case 0x2006: ppu_write(&nrom->ppu, PPUIO_ADDR, data); break;
        case 0x2007: ppu_write(&nrom->ppu, PPUIO_DATA, data); break;
        case 0x4014: // OAMDMA
            ppu_write_oam(&nrom->ppu, nrom->memory + (((uint16_t)data)<<8));
            nrom->cpu.cycles += nrom->cpu.cycles&2 + 513;
            break;
        case 0x4016:
            if (data&1)
            {
                nrom->controller_strobe = 0;
            }
            break;
        default:
            if (addr >= 0x8000) {
                assert(false);
            }
            if (nrom->prgsize == 1 && addr >= 0xC000) addr -= 0x4000;
            nrom->memory[addr] = data;
            break;
    }
}

void nrom_update_controller(struct nrom *nrom, struct controller_state cs)
{
    nrom->controller = cs;
}

static uint8_t _nrom_mem_read(void *mem, uint16_t addr)
{
    struct nrom *nrom = mem;
 
    if (addr >= 0x2000 && addr < 0x4000)
    {
        addr = 0x2000  +((addr-0x2000)%8);
    }

    switch (addr)
    {
        case 0x2002: return ppu_read(&nrom->ppu, PPUIO_STATUS);
        case 0x2004: return ppu_read(&nrom->ppu, PPUIO_OAMDATA);
        case 0x2007: return ppu_read(&nrom->ppu, PPUIO_DATA);
        case 0x4017:
            return 0;
        case 0x4016:

            if (nrom->controller_strobe != 8)
            {
                return nrom->controller.btns[nrom->controller_strobe++];
            }
            else
            {
                printf("STROBE: %d\n", nrom->controller_strobe);
                return 1;
            }

            break;
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

    return 0;
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
    out->controller_strobe = 8;

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

    uint32_t mirroring = ines[6]&1;
    
    out->ppu = ppu_mk(mirroring ? PPUMIR_VER : PPUMIR_HOR);
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

struct nrom_frame_result nrom_frame(struct nrom *nrom)
{
    struct ricoh_mem_interface mem = nrom_get_memory_interface(nrom);

    while (true)
    {
        if (nrom->cpu.cycles*3 < nrom->ppu.ticks)
        {
            struct instr_decoded decoded = ricoh_decode_instr(&nrom->decoder, &mem, nrom->cpu.pc);
            ricoh_run_instr(&nrom->cpu, decoded, &mem);
        }
        else
        {
            bool nmi_occured = ppu_cycle(&nrom->ppu, &mem);

            if (nmi_occured)
            {
                if (ppu_nmi_enabled(&nrom->ppu))
                {
                    ricoh_do_interrupt(&nrom->cpu, &mem, nrom_get_vector(nrom, VEC_NMI));
                }
                break;
            }
        }
    }

    struct nrom_frame_result result = { 0 };

    memcpy(result.screen, nrom->ppu.screen, sizeof result.screen);

    return result;
}