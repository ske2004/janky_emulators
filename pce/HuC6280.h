#include <stdint.h>
#include <stdbool.h>
#define ADDR_MODE_COUNT 13

typedef enum 
{
    ADC, AND, ASL,
    BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK, BVC, BVS,
    CLC, CLD, CLI, CLV, CMP, CPX, CPY,
    DEC, DEX, DEY,
    EOR,
    INC, INX, INY,
    JMP, JSR,
    LDA, LDX, LDY, LSR,
    NOP,
    ORA,
    PHA, PHP, PLA, PLP,
    ROL, ROR, RTI, RTS,
    SBC, SEC, SED, SEI, STA, STX, STY,
    TAX, TAY, TSX, TXA, TXS, TYA, _ICOUNT
} huc6280_instr;

typedef enum
{
    AM_ACC, AM_ABS, AM_ABX, AM_ABY, AM_IMM, AM_IMP, AM_IND, AM_XND, AM_INY, AM_REL, AM_ZPG, AM_ZPX, AM_ZPY
} huc6280_addr_mode;

typedef enum
{
    FLAG_CAR, FLAG_ZER, FLAG_INT, FLAG_DEC, FLAG_BRK, FLAG_BI5, FLAG_OFW, FLAG_NEG
} huc6280_flags;

typedef struct
{
    huc6280_instr Id;
    huc6280_addr_mode AddrMode;
    uint8_t Operand[2];
    size_t Size;
} huc6280_instr_decoded;

typedef struct
{
    uint8_t ITBL[256];
    uint8_t ATBL[256];
} huc6280_decoder;

typedef struct
{
    uint16_t PC;
    uint8_t A, X, Y, SP, Flags;
    uint64_t Cycles;

    bool IsCrashed;
} huc6280_state;

typedef struct
{
    void *Instance;
    uint8_t (*Get)(void *Instance, uint32_t Address);
    void (*Set)(void *Instance, uint32_t Address, uint8_t Byte);
} huc6280_bus;

huc6280_decoder HuC6280_MakeDecoder();
const char *HuC6280_Instr_To_Str(huc6280_instr instr);
huc6280_instr_decoded HuC6280_Decode_Instr(huc6280_decoder *decoder, huc6280_bus *mem, uint16_t addr);
void HuC6280_Format_Decoded_Instr(char *dest, huc6280_instr_decoded decoded);
void HuC6280_Do_Interrupt(
    huc6280_state *cpu,
    huc6280_bus *mem,
    uint16_t newpc
);
void HuC6280_Run_Instr(
    huc6280_state *cpu,
    huc6280_instr_decoded instr,
    huc6280_bus *mem
);

