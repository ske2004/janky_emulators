#include "neske.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

struct system system_init(struct mux_api apu_mux, struct ricoh_mem_interface mem)
{
    struct system system = { 0 };
    system.apu_mux = apu_mux;
    system.decoder = make_ricoh_decoder();
    system.ppu = ppu_mk();
    system.mem = mem;
    system_reset(&system);
    return system;
}

static void apu_write_safe(struct system *system, enum apu_reg reg, uint8_t val)
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

        case 0x4000: apu_write_safe(system, APU_PULSE1_DDLC_NNNN, data); break; // pulse 1
        case 0x4001: apu_write_safe(system, APU_PULSE1_EPPP_NSSS, data); break;
        case 0x4002: apu_write_safe(system, APU_PULSE1_LLLL_LLLL, data); break;
        case 0x4003: apu_write_safe(system, APU_PULSE1_LLLL_LHHH, data); break; 
        case 0x4004: apu_write_safe(system, APU_PULSE2_DDLC_NNNN, data); break; // pulse 2
        case 0x4005: apu_write_safe(system, APU_PULSE2_EPPP_NSSS, data); break;
        case 0x4006: apu_write_safe(system, APU_PULSE2_LLLL_LLLL, data); break;
        case 0x4007: apu_write_safe(system, APU_PULSE2_LLLL_LHHH, data); break;
        case 0x4008: apu_write_safe(system, APU_TRIANG_CRRR_RRRR, data); break; // triangle
        case 0x400A: apu_write_safe(system, APU_TRIANG_LLLL_LLLL, data); break;
        case 0x400B: apu_write_safe(system, APU_TRIANG_LLLL_LHHH, data); break;
        case 0x400C: apu_write_safe(system, APU_NOISER_XXLC_VVVV, data); break; // noise
        case 0x400E: apu_write_safe(system, APU_NOISER_MXXX_PPPP, data); break;
        case 0x400F: apu_write_safe(system, APU_NOISER_LLLL_LXXX, data); break;
        case 0x4015: apu_write_safe(system, APU_STATUS_IFXD_NT21, data); break; // status
        case 0x4017: apu_write_safe(system, APU_STATUS_MIXX_XXXX, data); break; // misc

        case 0x4014: // OAMDMA
            ppu_write_oam(&system->ppu, system->memory + (((uint16_t)data)<<8));
            system->cpu.cycles += system->cpu.cycles&2 + 513;
            break;
        case 0x4016:
            system->controller_strobe = data&1;
            if (system->controller_strobe)
            {
                system->controller_sr = 0;
            }
            break;
        default:
            system->memory[addr] = data;
            break;
    }
}

void system_update_controller(struct system *system, struct controller_state cs)
{
    system->controller = cs;
}

void system_generate_samples(struct system *system, uint16_t *samples, uint32_t count)
{
    system->apu_mux.lock(system->apu_mux.mux);
    system->apu.samples_read += count;
    apu_catchup_samples(&system->apu, count);
    apu_ring_read(&system->apu, samples, count);
    system->apu_mux.unlock(system->apu_mux.mux);
}

uint8_t system_mem_read(struct system *system, uint16_t addr)
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
        case 0x4017:
            return 0; // controller 2, not apu, confusing ya
        case 0x4016:
            if (system->controller_strobe)
            {
                system->controller_sr = 0;
            }
            if (system->controller_sr != 8)
            {
                // TODO: games like paperboy require 0x40 to be set, i'm lazy to figure out why right now
                return system->controller.btns[system->controller_sr++] | 0x40;
            }
            else
            {
                return 1;
            }
            break;
        default: 
            return system->memory[addr];
    }

    return 0;
}

uint16_t system_get_vector(struct system *system, enum vector vec)
{
    switch (vec)
    {
        case VEC_NMI: return   system->mem.get(system->mem.instance, 0xFFFA) | (system->mem.get(system->mem.instance, 0xFFFB) << 8);
        case VEC_RESET: return system->mem.get(system->mem.instance, 0xFFFC) | (system->mem.get(system->mem.instance, 0xFFFD) << 8);
        case VEC_IRQ: return   system->mem.get(system->mem.instance, 0xFFFE) | (system->mem.get(system->mem.instance, 0xFFFF) << 8);
    }

    return 0;
}


void system_reset(struct system *system)
{
    printf("system_reset\n");
    system->cpu = (struct ricoh_state){ 0 };
    system->cpu.pc = system_get_vector(system, VEC_RESET);
    printf("system_reset pc: %x\n", system->cpu.pc);
    system->cpu.flags = 0x24;
    system->cpu.sp = 0xFD;
    system->cpu.cycles = 7;
    system->apu = (struct apu){ 0 };
    apu_init(&system->apu);
    printf("system_reset done\n");
}

enum
{
    DEV_CPU,
    DEV_PPU,
};

struct system_frame_result system_frame(struct system *system)
{
    uint64_t cycles_start = system->cpu.cycles;

    while (!system->cpu.crash && (system->cpu.cycles-cycles_start) < 500000)
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
                struct instr_decoded decoded = ricoh_decode_instr(&system->decoder, &system->mem, system->cpu.pc);
                ricoh_run_instr(&system->cpu, decoded, &system->mem);
            }
            break;
        case DEV_PPU:
            {
                nmi_occured = ppu_cycle(&system->ppu, &system->mem);
            }
            break;
        }

        if (nmi_occured)
        {
            if (ppu_nmi_enabled(&system->ppu))
            {
                ricoh_do_interrupt(&system->cpu, &system->mem, system_get_vector(system, VEC_NMI));
            }
            break;
        }
    }

    struct system_frame_result result = { 0 };

    memcpy(result.screen, system->ppu.screen, sizeof result.screen);

    return result;
}

