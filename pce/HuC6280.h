#include <stdint.h>
#include <stdbool.h>

#define ADDR_MODE_COUNT 20
#define INSTRUCTION_COUNT 100

typedef uint32_t huc6280_addr;

typedef enum 
{
    ADC, AND, ASL,
    BBR0, BBR1, BBR2, BBR3, BBR4, BBR5, BBR6, BBR7,
    BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRA, BRK,
    BBS0, BBS1, BBS2, BBS3, BBS4, BBS5, BBS6, BBS7,
    BVC, BVS,
    CLC, CLD, CLI, CLV, CMP, CPX, CPY, CSH,
    DEC, DEX, DEY,
    EOR,
    INC, INX, INY,
    JMP, JSR,
    LDA, LDX, LDY, LSR,
    NOP,
    ORA,
    PHA, PHP, PHX, PHY, PLA, PLP, PLX, PLY,
    RMB0, RMB1, RMB2, RMB3, RMB4, RMB5, RMB6, RMB7,
    ROL, ROR, RTI, RTS,
    SBC, SEC, SED, SEI,
    SMB0, SMB1, SMB2, SMB3, SMB4, SMB5, SMB6, SMB7,
    STA, STP, STX, STY, STZ,
    TAM, TAX, TAY, TMA, TRB, TSB, TSX, TXA, TXS, TYA, 
    WAI
} huc6280_instr;

typedef enum
{
    AM_IMP, // Implied
    AM_IMM, // Immediate
    AM_ZPG, // Zero page
    AM_ZPX, // Zero page, X indexed
    AM_ZPY, // Zero page, Y indexed
    AM_ZPR, // Zero page, relative
    AM_ZPI, // Zero page, indirect
    AM_ZXI, // Zero page, X indexed, indirect
    AM_ZIY, // Zero page, indirect, Y indexed
    AM_ABS, // Absolute
    AM_ABX, // Absolute, X indexed
    AM_ABY, // Absolute, Y indexed
    AM_ABI, // Absolute, indirect
    AM_AXI, // Absolute, X indexed, indirect
    AM_REL, // Relative
    AM_IZP, // Immediate, zero page
    AM_IZX, // Immediate, zero page, X indexed
    AM_IAB, // Immediate, absolute
    AM_IAX, // Immediate, absolute, X indexed
    AM_ACC, // Accumulator
} huc6280_addr_mode;

typedef enum
{
    FLAG_CAR, FLAG_ZER, FLAG_INT, FLAG_DEC, FLAG_BRK, FLAG_MEM, FLAG_OFW, FLAG_NEG
} huc6280_flags;

typedef struct
{
    huc6280_instr Id;
    huc6280_addr_mode AddrMode;
    uint8_t Operand[6];
    size_t Size;
    uint8_t Cycles;
} huc6280_instr_decoded;

typedef struct
{
    uint8_t ITBL[256]; // Addressing mode table
    uint8_t ATBL[256]; // Instruction table
    uint8_t CTBL[256]; // Cycle table
} huc6280_decoder;

typedef struct
{
    uint16_t PC;
    uint8_t A, X, Y, SP, Flags;
    uint8_t MPR[8]; // Bank select for each 2k bank

    uint64_t Cycles;

    bool IsCrashed;
    bool IsFast; // if not set, multiply Cycles by 4
} huc6280_state;

typedef struct
{
    void *Instance;
    uint8_t (*Get)(void *Instance, huc6280_addr Address);
    void (*Set)(void *Instance, huc6280_addr Address, uint8_t Byte);
} huc6280_bus;

huc6280_decoder HuC6280_MakeDecoder();
huc6280_instr_decoded HuC6280_Decode_Instr(huc6280_decoder *decoder, huc6280_state *cpu, huc6280_bus *mem, uint16_t addr);
void HuC6280_Format_Decoded_Instr(char *dest, huc6280_instr_decoded decoded);
void HuC6280_PowerUp(huc6280_state *Cpu);
void HuC6280_Run_Instr(huc6280_state *Cpu, huc6280_instr_decoded Instr, huc6280_bus *Mem);
void HuC6280_Next_Instr(huc6280_state *Cpu, huc6280_decoder *Decoder, huc6280_bus *Mem);
