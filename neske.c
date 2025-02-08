#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "nrom.c"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <NES ROM>\n", argv[0]);
        return 1;
    }
    FILE *fp = fopen(argv[1], "rb");
    if (!fp)
    {
        perror(argv[1]);
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t *rom = malloc(fsize);
    fread(rom, 1, fsize, fp);
    fclose(fp);

    struct nrom nrom;
    if (nrom_load(rom, &nrom))
    {
        fprintf(stderr, "Invalid ROM file\n");
        return 1;
    }

    while (true)
    {
        nrom_frame(&nrom);
    }

    printf("Listing complete.\n");
}