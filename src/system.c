#include "neske.h"
#include <assert.h>
#include <assert.h>

struct system system_init(struct mux_api apu_mux)
{
    struct system system = { 0 };
    system.apu_mux = apu_mux;
    system.decoder = make_ricoh_decoder();
    system.ppu = ppu_mk();
    apu_init(&system.apu);
    return system;
}

static void apu_write_catchup(struct system *system, enum apu_reg reg, uint8_t val)
{
    system->apu_mux.lock(system->apu_mux.mux);
    apu_reg_write(&system->apu, reg, val);
    system->apu_mux.unlock(system->apu_mux.mux);
}

void system_mem_write(struct system *system, uint16_t addr, uint8_t data)
{
    if (addr >= 0x2000 && addr < 0x4000)
    {
        addr = 0x2000 + addr % 8;
    }

    switch (addr)
    {
        case 0x2000: ppu_write(&system->ppu, PPUIO_CTRL, data); break;
        case 0x2001: ppu_write(&system->ppu, PPUIO_MASK, data); break;
        case 0x2003: ppu_write(&system->ppu, PPUIO_OAMADDR, data); break;
        case 0x2004: ppu_write(&system->ppu, PPUIO_OAMDATA, data); break;
        case 0x2005: ppu_write(&system->ppu, PPUIO_SCROLL, data); break;
        case 0x2006: ppu_write(&system->ppu, PPUIO_ADDR, data); break;
        case 0x2007: ppu_write(&system->ppu, PPUIO_DATA, data); break;

        case 0x4000: apu_write_catchup(system, APU_PULSE1_DDLC_NNNN, data); break; // pulse 1
        case 0x4001: apu_write_catchup(system, APU_PULSE1_EPPP_NSSS, data); break;
        case 0x4002: apu_write_catchup(system, APU_PULSE1_LLLL_LLLL, data); break;
        case 0x4003: apu_write_catchup(system, APU_PULSE1_LLLL_LHHH, data); break; 
        case 0x4004: apu_write_catchup(system, APU_PULSE2_DDLC_NNNN, data); break; // pulse 2
        case 0x4005: apu_write_catchup(system, APU_PULSE2_EPPP_NSSS, data); break;
        case 0x4006: apu_write_catchup(system, APU_PULSE2_LLLL_LLLL, data); break;
        case 0x4007: apu_write_catchup(system, APU_PULSE2_LLLL_LHHH, data); break;
        case 0x4008: apu_write_catchup(system, APU_TRIANG_CRRR_RRRR, data); break; // triangle
        case 0x400A: apu_write_catchup(system, APU_TRIANG_LLLL_LLLL, data); break;
        case 0x400B: apu_write_catchup(system, APU_TRIANG_LLLL_LHHH, data); break;
        case 0x400C: apu_write_catchup(system, APU_NOISER_XXLC_VVVV, data); break; // noise
        case 0x400E: apu_write_catchup(system, APU_NOISER_MXXX_PPPP, data); break;
        case 0x400F: apu_write_catchup(system, APU_NOISER_LLLL_LXXX, data); break;
        case 0x4015: apu_write_catchup(system, APU_STATUS_IFXD_NT21, data); break; // status
        case 0x4017: apu_write_catchup(system, APU_STATUS_MIXX_XXXX, data); break; // misc

        case 0x4014: // OAMDMA
            ppu_write_oam(&system->ppu, system->memory + (((uint16_t)data)<<8));
            system->cpu.cycles += system->cpu.cycles&2 + 513;
            break;
        case 0x4016:
            if (data&1)
            {
                system->controller_strobe = 0;
            }
            break;
        default:
            // if (system->prgsize == 1 && addr >= 0xC000) addr -= 0x4000;
            system->memory[addr] = data;
            break;
    }
}

void system_update_controller(struct system *system, struct controller_state cs)
{
    system->controller = cs;
}

static uint8_t _system_mem_read(struct system *system, uint16_t addr)
{
    if (addr >= 0x2000 && addr < 0x4000)
    {
        addr = 0x2000  +((addr-0x2000)%8);
    }

    uint8_t val = 0;

    switch (addr)
    {
        case 0x2002: return ppu_read(&system->ppu, PPUIO_STATUS);
        case 0x2004: return ppu_read(&system->ppu, PPUIO_OAMDATA);
        case 0x2007: return ppu_read(&system->ppu, PPUIO_DATA);
        case 0x4015:
            system->apu_mux.lock(system->apu_mux.mux);
            val = apu_reg_read(&system->apu, APU_STATUS_IFXD_NT21);
            system->apu_mux.unlock(system->apu_mux.mux);
            return val;
            break;
        case 0x4017:
            return 0; // controller, not apu, confusing ya
        case 0x4016:

            if (system->controller_strobe != 8)
            {
                return system->controller.btns[system->controller_strobe++];
            }
            else
            {
                return 1;
            }

            break;
        default: 
            // if (system->prgsize == 1 && addr >= 0xC000) addr -= 0x4000;
            return system->memory[addr];
    }

    return 0;
}

uint16_t system_get_vector(struct system *system, enum vector vec)
{
    switch (vec)
    {
        case VEC_NMI: return   _system_mem_read(system, 0xFFFA) | (_system_mem_read(system, 0xFFFB) << 8);
        case VEC_RESET: return _system_mem_read(system, 0xFFFC) | (_system_mem_read(system, 0xFFFD) << 8);
        case VEC_IRQ: return   _system_mem_read(system, 0xFFFE) | (_system_mem_read(system, 0xFFFF) << 8);
    }

    return 0;
}


void system_reset(struct system *system, uint16_t pc)
{
    system->cpu = (struct ricoh_state){ 0 };
    system->cpu.pc = pc;
    system->cpu.flags = 0x24;
    system->cpu.sp = 0xFD;
    system->cpu.cycles = 7;
    system->apu = (struct apu){ 0 };
    apu_init(&system->apu);
}


struct ricoh_mem_interface system_get_memory_interface(struct nrom *nrom)
{
    return (struct ricoh_mem_interface){
        system,
        _system_mem_read,
        _system_mem_write,
    };
}

enum
{
    DEV_CPU,
    DEV_PPU,
};

struct system_frame_result nrom_frame(struct nrom *nrom)
{
    struct ricoh_mem_interface mem = system_get_memory_interface(nrom);

    uint64_t cycles_start = system->cpu.cycles;

    while (!system->cpu.crash && (nrom->cpu.cycles-cycles_start) < 500000)
    {
        bool nmi_occured = false;

        int dev = DEV_CPU;
        uint64_t devc = system->cpu.cycles;

        if (devc*3 >= system->ppu.cycles) {
            devc = system->ppu.cycles/3;
            dev = DEV_PPU;
        }

        switch (dev)
        {
        case DEV_CPU:
            {
                struct instr_decoded decoded = ricoh_decode_instr(&system->decoder, &mem, nrom->cpu.pc);
                ricoh_run_instr(&system->cpu, decoded, &mem);
            }
            break;
        case DEV_PPU:
            {
                nmi_occured = ppu_cycle(&system->ppu, &mem);
            }
            break;
        }

        if (nmi_occured)
        {
            if (ppu_nmi_enabled(&system->ppu))
            {
                ricoh_do_interrupt(&system->cpu, &mem, nrom_get_vector(nrom, VEC_NMI));
            }
            break;
        }
    }

    struct system_frame_result result = { 0 };

    memcpy(result.screen, system->ppu.screen, sizeof result.screen);

    return result;
}

