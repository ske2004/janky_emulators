#include "../neske.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

void _sr_reset(shift_register *sr)
{
    *sr = 1<<4;
}

struct shift_register_result _sr_write(shift_register *sr, uint8_t val)
{
    if (val & 0x80)
    {
        *sr = 1<<4;
        return (struct shift_register_result){ false };
    }

    bool overflow = *sr & 1;
    *sr = (*sr >> 1) | ((val & 1) << 4);

    if (overflow)
    {
        uint8_t val = *sr&0x1F;
        *sr = 1<<4;
        return (struct shift_register_result){ true, val };
    }

    return (struct shift_register_result){ false };
}

struct mapper_vtbl mmc1_vtbl = {
    .new                = mmc1_new,
    .free               = mmc1_free,
    .frame              = mmc1_frame,
    .generate_samples   = mmc1_generate_samples,
    .reset              = mmc1_reset,
    .crash              = mmc1_crash,
    .set_controller     = mmc1_set_controller,
};

enum ppu_mir _mmc1_get_mirroring(struct mmc1 *mapper)
{
    switch (mapper->reg_ctrl & 0x3)
    {
        case 0: return PPUMIR_ONE;
        case 1: return PPUMIR_ONE;
        case 2: return PPUMIR_VER;
        case 3: return PPUMIR_HOR;
    }

    return PPUMIR_HOR;
}

static uint8_t _mmc1_mem_read(void *mapper_data, uint16_t addr)
{
    struct mmc1 *mapper = (struct mmc1 *)mapper_data;

    size_t prg_bank_1;
    size_t prg_bank_2;

    switch ((mapper->reg_ctrl>>2)&0x3)
    {
        case 0:
        case 1:
            // 32kb chunk
            prg_bank_1 = mapper->reg_prg_bank>>1<<1;
            prg_bank_2 = prg_bank_1 + 1;
            break;
        case 2:
            // 16kb chunk, fix first bank at 0x8000
            prg_bank_1 = 0;
            prg_bank_2 = mapper->reg_prg_bank;
            break;
        case 3:
            // 16kb chunk, fix last bank at 0xC000
            prg_bank_1 = mapper->reg_prg_bank;
            prg_bank_2 = mapper->rom.prg_size/0x4000 - 1;
            break;
    }
    
    if (addr >= 0x8000 && addr < 0xC000)
    {
        return mapper->rom.prg[addr - 0x8000 + prg_bank_1*0x4000];
    }
    else if (addr >= 0xC000 && addr < 0xFFFF)
    {
        return mapper->rom.prg[addr - 0xC000 + prg_bank_2*0x4000];
    }

    return system_mem_read(&mapper->system, addr);
}

static void _mmc1_sync_registers(struct mmc1 *mapper)
{
    mapper->system.ppu.pins.mirroring_mode = _mmc1_get_mirroring(mapper);
    if (mapper->rom.chr_size > 0)
    {
        memcpy(mapper->system.ppu.pins.chr, mapper->rom.chr + mapper->reg_chr_bank_1*0x1000, 0x1000);
        memcpy(mapper->system.ppu.pins.chr + 0x1000, mapper->rom.chr + mapper->reg_chr_bank_2*0x1000, 0x1000);
    }
}

static void _mmc1_mem_write(void *mapper_data, uint16_t addr, uint8_t val)
{
    struct mmc1 *mapper = (struct mmc1 *)mapper_data;
    if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        struct shift_register_result sr_res = _sr_write(&mapper->shift_register, val);
        if (sr_res.do_write)
        {
            if (addr >= 0x8000 && addr <= 0x9FFF)
            {
                mapper->reg_ctrl = sr_res.value;
            }
            else if (addr >= 0xA000 && addr <= 0xBFFF)
            {
                mapper->reg_chr_bank_1 = sr_res.value;
            }
            else if (addr >= 0xC000 && addr <= 0xDFFF)
            {
                mapper->reg_chr_bank_2 = sr_res.value;
            }
            else if (addr >= 0xE000 && addr <= 0xFFFF)
            {
                mapper->reg_prg_bank = sr_res.value;
            }

            _mmc1_sync_registers(mapper);
        }
    }
    else
    {
        system_mem_write(&mapper->system, addr, val);
    }
}

void* mmc1_new(struct mapper_data data, struct mux_api apu_mux)
{
    struct mmc1 *mapper = calloc(1, sizeof(struct mmc1));
    assert(mapper != NULL);

    mapper->rom = mapper_rom_copy(&data);
    // Set PRG mode to fix last bank at 0xC000
    mapper->reg_ctrl |= 0x3 << 2;

    _sr_reset(&mapper->shift_register);

    mapper->system = system_init(apu_mux, (struct ricoh_mem_interface){
        .instance = mapper,
        .get = _mmc1_mem_read,
        .set = _mmc1_mem_write,
    });

    return mapper;
}

void mmc1_free(void *mapper_data)
{
    struct mmc1 *mapper = (struct mmc1 *)mapper_data;
    mapper_rom_free(&mapper->rom);
    free(mapper);
}

struct system_frame_result mmc1_frame(void *mapper_data)
{
    struct mmc1 *mapper = (struct mmc1 *)mapper_data;
    return system_frame(&mapper->system);
}

void mmc1_generate_samples(void *mapper_data, uint16_t *samples, uint32_t count)
{
    struct mmc1 *mapper = (struct mmc1 *)mapper_data;
    system_generate_samples(&mapper->system, samples, count);
}

void mmc1_reset(void *mapper_data)
{
    struct mmc1 *mapper = (struct mmc1 *)mapper_data;
    system_reset(&mapper->system);
}

bool mmc1_crash(void *mapper_data)
{
    struct mmc1 *mapper = (struct mmc1 *)mapper_data;
    return mapper->system.cpu.crash;
}

void mmc1_set_controller(void *mapper_data, struct controller_state controller)
{
    struct mmc1 *mapper = (struct mmc1 *)mapper_data;
    system_update_controller(&mapper->system, controller);
}