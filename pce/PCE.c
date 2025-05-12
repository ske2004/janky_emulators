#include "HuC6280.c"
#include "Memory.c"

typedef struct pce_emulator
{
    huc6280_state CPU;
    huc6280_decoder Decoder;
    huc6280_bus Bus;
    memory Mem;
} pce_emulator;

uint8_t PCE_Get(void *Instance, uint32_t Address)
{
    pce_emulator *Emulator = (pce_emulator *)Instance;
    return Memory_Read(&Emulator->Mem, Address);
}

void PCE_Set(void *Instance, uint32_t Address, uint8_t Value)
{
    pce_emulator *Emulator = (pce_emulator *)Instance;
    Memory_Write(&Emulator->Mem, Address, Value);
}

pce_emulator *PCE_Create(uint8_t *ROM, uint32_t Size)
{
    pce_emulator *Emulator = (pce_emulator *)calloc(1, sizeof(pce_emulator));

    Emulator->Decoder = HuC6280_MakeDecoder();
    Emulator->Mem = CreateMemory(ROM, Size);
    Emulator->Bus.Get = PCE_Get;
    Emulator->Bus.Set = PCE_Set;
    Emulator->Bus.Instance = Emulator;

    return Emulator;
}

void PCE_Run(pce_emulator *Emulator)
{
    HuC6280_Dump_Known_Ops(&Emulator->Decoder);

    // HuC6280_PowerUp(&Emulator->CPU);
    // while (Emulator->CPU.IsCrashed == false)
    // {
    //     HuC6280_Next_Instr(&Emulator->CPU, &Emulator->Decoder, &Emulator->Bus);
    // }
}

void PCE_Destroy(pce_emulator *Emulator)
{
    free(Emulator);
}