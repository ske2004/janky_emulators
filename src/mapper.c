#include "neske.h"

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
    data.mapper_number = (ines[6] >> 4) | (ines[7] & 0xF0);
    data.mirroring = !(ines[6] & 1);

    return data;
}

struct mapper mapper_init(uint8_t *ines, struct mux_api apu_mux)
{
    struct mapper mapper = { 0 };

    struct mapper_data data = mapper_get_data(ines);

    if (!data.is_valid)
    {
        return mapper;
    }

    mapper.system = system_init(apu_mux);

    switch (data.mapper_number)
    {
        case 0:
            mapper.mapper_data = nrom_init(data);
            break;
    }

    return mapper;
}

void mapper_reset(struct mapper *mapper);
void mapper_set_controller(struct mapper *mapper, struct controller_state controller);
void mapper_free(struct mapper *mapper);
