#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "ricoh.c"

uint8_t simple_mem_read(void *instance, uint16_t addr)
{
    if (addr >= 0xC000) {
        addr -= 0x4000;
    }
    return ((uint8_t*)instance)[addr];
}

void simple_mem_write(void *instance, uint16_t addr, uint8_t val)
{
    if (addr >= 0xC000) {
        addr -= 0x4000;
    }
    ((uint8_t*)instance)[addr] = val; 
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <6502 binary>\n", argv[0]);
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
    uint8_t *mem = calloc(1, 1<<16);
    fread(mem, 1, fsize, fp);
    fclose(fp);

    uint8_t *prg = calloc(1, 1<<16);
    memcpy(prg+0x8000, mem+16, mem[4]*16384+mem[5]*8192);
    
    struct ricoh_decoder decoder = make_ricoh_decoder();
    struct ricoh_state cpu = { 0 };
    cpu.pc = 0xC000;
    cpu.flags = 0x24;
    cpu.sp = 0xFD;
    cpu.cycles = 7;
    struct ricoh_mem_interface memif = {
        prg,
        simple_mem_read,
        simple_mem_write
    };


    while (true)
    {
        uint16_t start = cpu.pc;
        struct instr_decoded decoded = ricoh_decode_instr(&decoder, &memif, cpu.pc);
        
        char buf[1024] = { 0 };
        ricoh_format_decoded_instr(buf, decoded);

        printf("%04X  ", cpu.pc);
        switch (decoded.size)
        {
            case 1: printf("%02X        ", memif.get(memif.instance, start)); break;
            case 2: printf("%02X %02X     ", memif.get(memif.instance, start), memif.get(memif.instance, start+1)); break;
            case 3: printf("%02X %02X %02X  ", memif.get(memif.instance, start), memif.get(memif.instance, start+1), memif.get(memif.instance, start+2)); break;
        }

        printf("%s", buf);
        for (size_t i = 0; i < 32-strlen(buf); i++)
        {
            printf(" ");
        }
        printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%d\n", cpu.a, cpu.x, cpu.y, cpu.flags, cpu.sp, cpu.cycles);

        ricoh_run_instr(&cpu, decoded, &memif);
    }

    printf("Listing complete.\n");
}