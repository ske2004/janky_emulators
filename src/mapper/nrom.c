#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../neske.h"

struct mapper_vtbl nrom_vtbl = {
    .new = nrom_new,
    .free = nrom_free,
    .frame = nrom_frame,
    .generate_samples = nrom_generate_samples,
    .reset = nrom_reset,
    .crash = nrom_crash,
    .set_controller = nrom_set_controller,
};

uint16_t map_memory_addr(struct nrom *mapper, uint16_t addr)
{
    if (mapper->is_mirrored && addr >= 0xC000)
    {
        return addr - 0x4000;
    }
    return addr;
}

static uint8_t _nrom_mem_read(void *mapper_data, uint16_t addr)
{
    struct nrom *mapper = (struct nrom *)mapper_data;
    uint16_t addr_mapped = map_memory_addr(mapper, addr);

    if (addr_mapped >= 0x8000 && addr_mapped <= 0xFFFF)
    {
        return mapper->rom[addr_mapped - 0x8000];
    }
    return system_mem_read(&mapper->system, addr_mapped);
}

static void _nrom_mem_write(void *mapper_data, uint16_t addr, uint8_t val)
{
    struct nrom *mapper = (struct nrom *)mapper_data;
    system_mem_write(&mapper->system, map_memory_addr(mapper, addr), val);
}

void* nrom_new(struct mapper_data data, struct mux_api apu_mux)
{
    size_t prg_offset = 16;
    size_t chr_offset = prg_offset + data.prg_size;

    if (data.prg_banks > 2)
    {
        return NULL;
    }

    if (data.chr_banks > 1)
    {
        return NULL;
    }

    struct nrom *mapper = malloc(sizeof(struct nrom));
    assert(mapper != NULL);

    mapper->is_mirrored = data.prg_banks == 1;
    mapper->rom = malloc(data.prg_size);
    memcpy(mapper->rom, data.ines+prg_offset, data.prg_size);

    mapper->system = system_init(apu_mux, (struct ricoh_mem_interface){
        .instance = mapper,
        .get = _nrom_mem_read,
        .set = _nrom_mem_write,
    });
    mapper->system.ppu.pins.mirroring_mode = data.mirroring;
    memcpy(mapper->system.ppu.pins.chr, data.ines+chr_offset, data.chr_size);

    return mapper;
}

void nrom_free(void *mapper_data)
{
    struct nrom *mapper = (struct nrom *)mapper_data;
    free(mapper->rom);
    free(mapper);
}

struct system_frame_result nrom_frame(void *mapper_data)
{
    struct nrom *mapper = (struct nrom *)mapper_data;
    return system_frame(&mapper->system);
}

void nrom_generate_samples(void *mapper_data, uint16_t *samples, uint32_t count)
{
    struct nrom *mapper = (struct nrom *)mapper_data;
    system_generate_samples(&mapper->system, samples, count);
}

void nrom_reset(void *mapper_data)
{
    struct nrom *mapper = (struct nrom *)mapper_data;
    system_reset(&mapper->system);
}

bool nrom_crash(void *mapper_data)
{
    struct nrom *mapper = (struct nrom *)mapper_data;
    return mapper->system.cpu.crash;
}

void nrom_set_controller(void *mapper_data, struct controller_state controller)
{
    struct nrom *mapper = (struct nrom *)mapper_data;
    system_update_controller(&mapper->system, controller);
}
