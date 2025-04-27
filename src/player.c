#include "neske.h"
#include <stdio.h>
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

    return data;
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
        case 0:
            player.vtbl = &nrom_vtbl;
            break;
        default:
            return player;
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
    struct system_frame_result result = player->vtbl->frame(player->mapper_data);
    return result;
}

void player_generate_samples(struct player *player, uint16_t *samples, uint32_t count)
{
    player->vtbl->generate_samples(player->mapper_data, samples, count);
}

void player_reset(struct player *player)
{
    printf("player_reset\n");
    player->vtbl->reset(player->mapper_data);
    printf("player_reset done\n");
}

void player_set_controller(struct player *player, struct controller_state controller)
{
    player->vtbl->set_controller(player->mapper_data, controller);
}

void player_free(struct player *player)
{
    player->is_valid = false;
    player->vtbl->free(player->mapper_data);
}

bool player_crash(struct player *player)
{
    return player->vtbl->crash(player->mapper_data);
}
