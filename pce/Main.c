#include "PCE.c"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <rom_path>\n", argv[0]);
        return 1;
    }

    raw_file Raw = Load_Raw(argv[1]);
    if (Raw.Data == NULL)
    {
        printf("Failed to load ROM\n");
        return 1;
    }

    pce_emulator *Emulator = PCE_Create(Raw.Data, Raw.Size);
    PCE_Run(Emulator);

    return 0;
}


