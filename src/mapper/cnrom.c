#include "../neske.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

struct mapper_vtbl cnrom_vtbl = {
    .new                = cnrom_new,
    .free               = cnrom_free,
    .frame              = cnrom_frame,
    .generate_samples   = cnrom_generate_samples,
    .reset              = cnrom_reset,
    .crash              = cnrom_crash,
    .set_controller     = cnrom_set_controller,
    .get_system         = cnrom_get_system,
};

static uint8_t _cnrom_mem_read(void *mapper_data, uint16_t addr)
{
    struct cnrom *mapper = (struct cnrom *)mapper_data;

    if (addr >= 0x8000 && addr < 0xFFFF)
    {
        return mapper->rom.prg[addr - 0x8000];
    }

    return system_mem_read(&mapper->system, addr);
}

static void _cnrom_update_chr(struct cnrom *mapper)
{
    memcpy(mapper->system.ppu.pins.chr, mapper->rom.chr+mapper->chr_bank*0x2000, 0x2000);
}

static void _cnrom_mem_write(void *mapper_data, uint16_t addr, uint8_t val)
{
    struct cnrom *mapper = (struct cnrom *)mapper_data;
    if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        mapper->chr_bank = val&3;
        _cnrom_update_chr(mapper);
    }
    else
    {
        system_mem_write(&mapper->system, addr, val);
    }
}

void* cnrom_new(struct mapper_data data, struct mux_api apu_mux)
{
    struct cnrom *mapper = calloc(1, sizeof(struct cnrom));
    assert(mapper != NULL);

    mapper->rom = mapper_rom_copy(&data);

    mapper->system = system_init(apu_mux, (struct ricoh_mem_interface){
        .instance = mapper,
        .get = _cnrom_mem_read,
        .set = _cnrom_mem_write,
    });
    
    mapper->system.ppu.pins.mirroring_mode = data.mirroring;
    memcpy(mapper->system.ppu.pins.chr, mapper->rom.chr+mapper->chr_bank*0x2000, 0x2000);
    
    return mapper;
}

void cnrom_free(void *mapper_data)
{
    struct cnrom *mapper = (struct cnrom *)mapper_data;
    mapper_rom_free(&mapper->rom);
    free(mapper);
}

struct system_frame_result cnrom_frame(void *mapper_data)
{
    struct cnrom *mapper = (struct cnrom *)mapper_data;
    return system_frame(&mapper->system);
}

void cnrom_generate_samples(void *mapper_data, uint16_t *samples, uint32_t count)
{
    struct cnrom *mapper = (struct cnrom *)mapper_data;
    system_generate_samples(&mapper->system, samples, count);
}

void cnrom_reset(void *mapper_data)
{
    struct cnrom *mapper = (struct cnrom *)mapper_data;
    system_reset(&mapper->system);
}

bool cnrom_crash(void *mapper_data)
{
    struct cnrom *mapper = (struct cnrom *)mapper_data;
    return mapper->system.cpu.crash;
}

void cnrom_set_controller(void *mapper_data, struct controller_state controller)
{
    struct cnrom *mapper = (struct cnrom *)mapper_data;
    system_update_controller(&mapper->system, controller);
}

struct system *cnrom_get_system(void *mapper_data)
{
    struct cnrom *mapper = (struct cnrom *)mapper_data;
    return &mapper->system;
}