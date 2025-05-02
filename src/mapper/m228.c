#include "../neske.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

struct mapper_vtbl m228_vtbl = {
    .new                = m228_new,
    .free               = m228_free,
    .frame              = m228_frame,
    .generate_samples   = m228_generate_samples,
    .reset              = m228_reset,
    .crash              = m228_crash,
    .set_controller     = m228_set_controller,
    .get_system         = m228_get_system,
};

struct parsed_data
{
    size_t chr_bank;
    size_t prg_addr_1;
    size_t prg_addr_2;
    enum ppu_mir mirroring;
};

static struct parsed_data _parse_data(struct m228 *mapper)
{
    struct parsed_data data = { 0 };
    bool prg_same = mapper->reg_addr & (1<<5);
    size_t prg_chip = (mapper->reg_addr >> 0xB)&3;
    // Chip 2 is unused, but we need to use the chip 2 address to get the correct PRG bank
    if (prg_chip == 3) prg_chip = 2;

    size_t prg_bank  = (mapper->reg_addr >> 6) & 0x1F;
    data.chr_bank = mapper->reg_data & 0x3 | ((mapper->reg_addr & 0xF)<<2
    
    );
    data.mirroring = (mapper->reg_addr & (1<<0xD)) ? PPUMIR_HOR : PPUMIR_VER;

    if (prg_same)
    {
        data.prg_addr_1 = prg_bank * 0x4000 + prg_chip * 0x80000;
        data.prg_addr_2 = data.prg_addr_1;
    }
    else
    {
        data.prg_addr_1 = ((prg_bank>>1) * 0x8000) + prg_chip * 0x80000;
        data.prg_addr_2 = data.prg_addr_1 + 0x4000;
    }

    return data;
}

static uint8_t _m228_mem_read(void *mapper_data, uint16_t addr)
{
    struct m228 *mapper = (struct m228 *)mapper_data;
    struct parsed_data data = _parse_data(mapper);

    if (addr >= 0x8000 && addr < 0xC000)
    {
        return mapper->rom.prg[addr - 0x8000 + data.prg_addr_1];
    }
    else if (addr >= 0xC000 && addr <= 0xFFFF)
    {
        return mapper->rom.prg[addr - 0xC000 + data.prg_addr_2];
    }

    return system_mem_read(&mapper->system, addr);
}

static void _update_chr_and_mirroring(struct m228 *mapper)
{
    struct parsed_data data = _parse_data(mapper);

    memcpy(mapper->system.ppu.pins.chr, mapper->rom.chr+data.chr_bank*0x2000, 0x2000);
    mapper->system.ppu.pins.mirroring_mode = data.mirroring;
}

static void _m228_mem_write(void *mapper_data, uint16_t addr, uint8_t val)
{
    struct m228 *mapper = (struct m228 *)mapper_data;
    if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        mapper->reg_data = val;
        mapper->reg_addr = addr;
        _update_chr_and_mirroring(mapper);
    }
    else
    {
        system_mem_write(&mapper->system, addr, val);
    }
}

void* m228_new(struct mapper_data data, struct mux_api apu_mux)
{
    struct m228 *mapper = calloc(1, sizeof(struct m228));
    assert(mapper != NULL);

    printf("MAKE YOUR SELECTION, NOW!\n");

    mapper->rom = mapper_rom_copy(&data);

    mapper->system = system_init(apu_mux, (struct ricoh_mem_interface){
        .instance = mapper,
        .get = _m228_mem_read,
        .set = _m228_mem_write,
    });
    
    _m228_mem_write(mapper, 0x8000, 0x00);

    return mapper;
}

void m228_free(void *mapper_data)
{
    struct m228 *mapper = (struct m228 *)mapper_data;
    mapper_rom_free(&mapper->rom);
    free(mapper);
}

struct system_frame_result m228_frame(void *mapper_data)
{
    struct m228 *mapper = (struct m228 *)mapper_data;
    return system_frame(&mapper->system);
}

void m228_generate_samples(void *mapper_data, uint16_t *samples, uint32_t count)
{
    struct m228 *mapper = (struct m228 *)mapper_data;
    system_generate_samples(&mapper->system, samples, count);
}

void m228_reset(void *mapper_data)
{
    struct m228 *mapper = (struct m228 *)mapper_data;
    _m228_mem_write(mapper, 0x8000, 0x00);
    system_reset(&mapper->system);
}

bool m228_crash(void *mapper_data)
{
    struct m228 *mapper = (struct m228 *)mapper_data;
    return mapper->system.cpu.crash;
}

void m228_set_controller(void *mapper_data, struct controller_state controller)
{
    struct m228 *mapper = (struct m228 *)mapper_data;
    system_update_controller(&mapper->system, controller);
}

struct system *m228_get_system(void *mapper_data)
{
    struct m228 *mapper = (struct m228 *)mapper_data;
    return &mapper->system;
}