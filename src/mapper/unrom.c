#include "../neske.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

struct mapper_vtbl unrom_vtbl = {
    .new                = unrom_new,
    .free               = unrom_free,
    .frame              = unrom_frame,
    .generate_samples   = unrom_generate_samples,
    .reset              = unrom_reset,
    .crash              = unrom_crash,
    .set_controller     = unrom_set_controller,
    .get_system         = unrom_get_system,
};


static uint8_t _unrom_mem_read(void *mapper_data, uint16_t addr)
{
    struct unrom *mapper = (struct unrom *)mapper_data;

    size_t prg_bank_1 = mapper->prg_select;
    size_t prg_bank_2 = mapper->rom.prg_size/0x4000 - 1;
    
    if (addr >= 0x8000 && addr < 0xC000)
    {
        return mapper->rom.prg[addr - 0x8000 + prg_bank_1*0x4000];
    }
    else if (addr >= 0xC000 && addr <= 0xFFFF)
    {
        return mapper->rom.prg[addr - 0xC000 + prg_bank_2*0x4000];
    }

    return system_mem_read(&mapper->system, addr);
}

static void _unrom_mem_write(void *mapper_data, uint16_t addr, uint8_t val)
{
    struct unrom *mapper = (struct unrom *)mapper_data;
    if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        mapper->prg_select = val & 0x7;
    }
    else
    {
        system_mem_write(&mapper->system, addr, val);
    }
}

void* unrom_new(struct mapper_data data, struct mux_api apu_mux)
{
    struct unrom *mapper = calloc(1, sizeof(struct unrom));
    assert(mapper != NULL);

    mapper->rom = mapper_rom_copy(&data);

    mapper->system = system_init(apu_mux, (struct ricoh_mem_interface){
        .instance = mapper,
        .get = _unrom_mem_read,
        .set = _unrom_mem_write,
    });
    
    mapper->system.ppu.pins.mirroring_mode = data.mirroring;

    return mapper;
}

void unrom_free(void *mapper_data)
{
    struct unrom *mapper = (struct unrom *)mapper_data;
    mapper_rom_free(&mapper->rom);
    free(mapper);
}

struct system_frame_result unrom_frame(void *mapper_data)
{
    struct unrom *mapper = (struct unrom *)mapper_data;
    return system_frame(&mapper->system);
}

void unrom_generate_samples(void *mapper_data, uint16_t *samples, uint32_t count)
{
    struct unrom *mapper = (struct unrom *)mapper_data;
    system_generate_samples(&mapper->system, samples, count);
}

void unrom_reset(void *mapper_data)
{
    struct unrom *mapper = (struct unrom *)mapper_data;
    system_reset(&mapper->system);
}

bool unrom_crash(void *mapper_data)
{
    struct unrom *mapper = (struct unrom *)mapper_data;
    return mapper->system.cpu.crash;
}

void unrom_set_controller(void *mapper_data, struct controller_state controller)
{
    struct unrom *mapper = (struct unrom *)mapper_data;
    system_update_controller(&mapper->system, controller);
}

struct system *unrom_get_system(void *mapper_data)
{
    struct unrom *mapper = (struct unrom *)mapper_data;
    return &mapper->system;
}