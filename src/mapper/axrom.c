// Mapper used by Rareware

#include "../neske.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

struct mapper_vtbl axrom_vtbl = {
    .new                = axrom_new,
    .free               = axrom_free,
    .frame              = axrom_frame,
    .generate_samples   = axrom_generate_samples,
    .reset              = axrom_reset,
    .crash              = axrom_crash,
    .set_controller     = axrom_set_controller,
    .get_system         = axrom_get_system,
};

static uint8_t _axrom_mem_read(void *mapper_data, uint16_t addr)
{
    struct axrom *mapper = (struct axrom *)mapper_data;

    size_t prg_addr = mapper->prg_bank*0x8000;

    if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        return mapper->rom.prg[addr - 0x8000 + prg_addr];
    }

    return system_mem_read(&mapper->system, addr);
}

static void _axrom_mem_write(void *mapper_data, uint16_t addr, uint8_t val)
{
    struct axrom *mapper = (struct axrom *)mapper_data;
    if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        bool second_screen = (val>>4)&1;
        mapper->prg_bank = val&0x7;
        mapper->system.ppu.pins.mirroring_mode = second_screen ? PPUMIR_ONE_ALT : PPUMIR_ONE;
    }
    else
    {
        system_mem_write(&mapper->system, addr, val);
    }
}

void* axrom_new(struct mapper_data data, struct mux_api apu_mux)
{
    struct axrom *mapper = calloc(1, sizeof(struct axrom));
    assert(mapper != NULL);

    mapper->rom = mapper_rom_copy(&data);

    mapper->system = system_init(apu_mux, (struct ricoh_mem_interface){
        .instance = mapper,
        .get = _axrom_mem_read,
        .set = _axrom_mem_write,
    });
    
    mapper->system.ppu.pins.mirroring_mode = PPUMIR_ONE;
    return mapper;
}

void axrom_free(void *mapper_data)
{
    struct axrom *mapper = (struct axrom *)mapper_data;
    mapper_rom_free(&mapper->rom);
    free(mapper);
}

struct system_frame_result axrom_frame(void *mapper_data)
{
    struct axrom *mapper = (struct axrom *)mapper_data;
    return system_frame(&mapper->system);
}

void axrom_generate_samples(void *mapper_data, uint16_t *samples, uint32_t count)
{
    struct axrom *mapper = (struct axrom *)mapper_data;
    system_generate_samples(&mapper->system, samples, count);
}

void axrom_reset(void *mapper_data)
{
    struct axrom *mapper = (struct axrom *)mapper_data;
    system_reset(&mapper->system);
}

bool axrom_crash(void *mapper_data)
{
    struct axrom *mapper = (struct axrom *)mapper_data;
    return mapper->system.cpu.crash;
}

void axrom_set_controller(void *mapper_data, struct controller_state controller)
{
    struct axrom *mapper = (struct axrom *)mapper_data;
    system_update_controller(&mapper->system, controller);
}

struct system *axrom_get_system(void *mapper_data)
{
    struct axrom *mapper = (struct axrom *)mapper_data;
    return &mapper->system;
}