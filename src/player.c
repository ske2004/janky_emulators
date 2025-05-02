#include "neske.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct mapper_data mapper_get_data(uint8_t *ines)
{
    struct mapper_data data = { 0 };

    if (!(ines[0] == 'N' && ines[1] == 'E' && ines[2] == 'S' && ines[3] == 0x1A))
    {
        return data;
    }
    
    data.is_valid = true;
    data.ines = ines;

    data.prg_banks = ines[4];
    data.chr_banks = ines[5];
    data.prg_size = data.prg_banks*0x4000;
    data.chr_size = data.chr_banks*0x2000;
    data.mapper_number = (ines[6] >> 4) | (ines[7] & 0xF0);
    data.mirroring = (ines[6] & 1) ? PPUMIR_VER : PPUMIR_HOR;

    printf("Mapper number: %d\n", data.mapper_number);

    assert(data.prg_size != 0);

    return data;
}

struct mapper_rom mapper_rom_copy(struct mapper_data *data)
{
    uint8_t *data_copy = malloc(data->prg_size+data->chr_size);
    if (!data_copy)
    {
        return (struct mapper_rom){ 0 };
    }

    memcpy(data_copy, data->ines+16, data->prg_size+data->chr_size);

    return (struct mapper_rom){ data->prg_size, data->chr_size, data_copy, data_copy+data->prg_size };
}

void mapper_rom_free(struct mapper_rom *rom)
{
    free(rom->prg);
}

struct player player_init(uint8_t *ines, struct mux_api apu_mux)
{
    printf("player_init\n");
    struct player player = { 0 };

    struct mapper_data data = mapper_get_data(ines);

    if (!data.is_valid)
    {
        return player;
    }

    switch (data.mapper_number)
    {
        case 0: player.vtbl = &nrom_vtbl; break;
        case 1: player.vtbl = &mmc1_vtbl; break;
        case 2: player.vtbl = &unrom_vtbl; break;
        case 3: player.vtbl = &cnrom_vtbl; break;
        case 7: player.vtbl = &axrom_vtbl; break;
        case 228: player.vtbl = &m228_vtbl; break;
        default: return player;
    }

    player.mapper_data = player.vtbl->new(data, apu_mux);

    if (!player.mapper_data)
    {
        return player;
    }

    player.is_valid = true;

    printf("player_init done\n");

    return player;
}

struct system_frame_result player_frame(struct player *player)
{
    if (player->is_valid)
    {
        return player->vtbl->frame(player->mapper_data);
    }

    return (struct system_frame_result){ 0 };
}

void player_generate_samples(struct player *player, uint16_t *samples, uint32_t count)
{
    if (player->is_valid)
    {
        player->vtbl->generate_samples(player->mapper_data, samples, count);
    }
}

void player_reset(struct player *player)
{
    if (player->is_valid)
    {
        printf("player_reset\n");
        player->vtbl->reset(player->mapper_data);
        printf("player_reset done\n");
    }
}

void player_set_controller(struct player *player, struct controller_state controller)
{
    if (player->is_valid)
    {
        player->vtbl->set_controller(player->mapper_data, controller);
    }
}

void player_free(struct player *player)
{
    if (player->is_valid)
    {
        player->is_valid = false;
        player->vtbl->free(player->mapper_data);
    }
}

bool player_crash(struct player *player)
{
    if (player->is_valid)
    {
        return player->vtbl->crash(player->mapper_data);
    }

    return false;
}

struct system *player_get_system(struct player *player)
{
    if (player->is_valid)
    {
        return player->vtbl->get_system(player->mapper_data);
    }

    return NULL;
}
