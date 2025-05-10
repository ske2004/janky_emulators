#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ADDR_MASK 0x1FFFFF
#define BANK_SIZE 0x2000
#define BANK_MASK 0x1FFF

#define ADDR_WRITABLE(addr) (addressing){(addr), true}
#define ADDR_READONLY(addr) (addressing){(addr), false}
#define ADDR_NULL           (addressing){NULL, false}
#define ADDR_IS_NULL(addr)  ((addr).Ptr == NULL)

typedef struct {
    uint8_t *Ptr;
    bool Writable;
} addressing;

typedef struct {
    bool IsMirrored;
    uint8_t *ROM;
    uint32_t ROM_Size;
    uint8_t RAM[0x2000];
} memory;

memory CreateMemory(uint8_t *ROM, uint32_t Size)
{
    assert(ROM != NULL); 
    assert(Size > BANK_SIZE);
    assert((Size & (BANK_SIZE - 1)) == 0); // misaligned rom

    uint8_t *ROM_Copy = (uint8_t *)malloc(Size);
    memcpy(ROM_Copy, ROM, Size);

    memory Mem;
    Mem.ROM = ROM_Copy;
    Mem.ROM_Size = Size;
    Mem.IsMirrored = true; // todo: this is just an assumption
    return Mem;
}

addressing Memory_Map(memory *Mem, uint32_t Address)
{
    Address &= ADDR_MASK;

    uint8_t Page = Address >> 13;

    if (Page >= 0x00 && Page <= 0x7F)
    {
        if (Mem->IsMirrored)
        {
            return ADDR_READONLY(&Mem->ROM[Address & (Mem->ROM_Size - 1)]);
        }
        else
        {
            if (Address < Mem->ROM_Size)
            {
                return ADDR_READONLY(&Mem->ROM[Address]);
            }
            else
            {
                return ADDR_NULL;
            }
        }
    }
    else if (Page >= 0xF8 && Page <= 0xFB)
    {
        return ADDR_WRITABLE(&Mem->RAM[Address & BANK_MASK]);
    }
    
    return ADDR_NULL;
}

uint8_t Memory_Read(memory *Mem, uint32_t Address)
{
    addressing Page = Memory_Map(Mem, Address);
    if (ADDR_IS_NULL(Page))
    {
        return 0xFF;
    }

    return *Page.Ptr;
}

void Memory_Write(memory *Mem, uint32_t Address, uint8_t Value)
{
    addressing Page = Memory_Map(Mem, Address);

    if (!ADDR_IS_NULL(Page) && Page.Writable)
    {
        *Page.Ptr = Value;
    }
}

void Memory_Free(memory *Mem)
{
    free(Mem->ROM);
}

typedef struct
{
    size_t Size;
    uint8_t *Data;
} raw_file;

raw_file Load_Raw(const char *Path)
{
    FILE *File = fopen(Path, "rb");
    if (File == NULL)
    {
        return (raw_file){0, NULL};
    }

    fseek(File, 0, SEEK_END);
    size_t Size = ftell(File);
    fseek(File, 0, SEEK_SET);
    uint8_t *Data = (uint8_t *)malloc(Size);
    fread(Data, 1, Size, File);
    fclose(File);

    raw_file Raw;
    Raw.Size = Size;
    Raw.Data = Data;
    return Raw;
}

void Free_Raw(raw_file *Raw)
{
    free(Raw->Data);
    Raw->Data = NULL;
    Raw->Size = 0;
}