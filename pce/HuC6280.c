// This CPU is modified version of 65C02 by WDC.
// I based this on ricoh.c from neske.

#include "HuC6280.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// 16 bites - 13 bits = 3 bits = 8 banks
#define CPU_BANK_SHIFT 13
#define CPU_BANK_SIZE 0x2000
// 0x2000 - 1 :3
#define CPU_BANK_MASK 0x1FFF
#define CPU_BANK_SEL(Addr) ((Addr) << CPU_BANK_SHIFT)

typedef struct
{
    uint8_t Opcode;
    huc6280_addr_mode AddrMode;
    huc6280_instr Instr;
    uint8_t Cycles;
} huc6280_opc_mapping;

const char *HUC6280_INSTR_TO_STR[] =
{
    "ADC","AND","ASL",
    "BBR0","BBR1","BBR2","BBR3","BBR4","BBR5","BBR6","BBR7",
    "BCC","BCS","BEQ","BIT","BMI","BNE","BPL","BRA","BRK",
    "BBS0","BBS1","BBS2","BBS3","BBS4","BBS5","BBS6","BBS7",
    "BVC","BVS",
    "CLC","CLD","CLI","CLV","CMP","CPX","CPY", "CSH",
    "DEC","DEX","DEY",
    "EOR",
    "INC","INX","INY",
    "JMP","JSR",
    "LDA","LDX","LDY","LSR",
    "NOP",
    "ORA",
    "PHA","PHP","PHX","PHY","PLA","PLP","PLX","PLY",
    "RMB0","RMB1","RMB2","RMB3","RMB4","RMB5","RMB6","RMB7",
    "ROL","ROR","RTI","RTS",
    "SBC","SEC","SED","SEI",
    "SMB0","SMB1","SMB2","SMB3","SMB4","SMB5","SMB6","SMB7",
    "STA","STP","STX","STY","STZ",
    "TAM","TAX","TAY","TMA","TRB","TSB","TSX","TXA","TXS","TYA",
    "WAI",
    "???"
};

const size_t HUC6280_INSTR_TO_STR_LEN = sizeof(HUC6280_INSTR_TO_STR)/sizeof(HUC6280_INSTR_TO_STR[0]);

const huc6280_opc_mapping HUC6280_OPC_DECODE_INFO[] =
{
    {0x00, AM_IMP, BRK, 7},
    {0x53, AM_IMP, TAM, 5},

    {0x64, AM_ZPG, STZ, 4},

    {0x78, AM_IMP, SEI, 2},

    {0x9A, AM_IMP, TXS, 2},
    
    {0xA2, AM_IMM, LDX, 2},
    {0xA9, AM_IMM, LDA, 2},

    {0xD4, AM_IMP, CSH, 3},
    {0xD8, AM_IMP, CLD, 2},
};  

void _SetFlag(huc6280_state *CPU, uint8_t Flag, uint8_t Value)
{
    if (Value)
    {
        CPU->Flags |= (1 << Flag);
    }
    else
    {
        CPU->Flags &= ~(1 << Flag);
    }
}

// Maps 16 bit address to 21 bit address
uint32_t _MMU_Map(huc6280_state *CPU, uint16_t Addr)
{
    uint8_t Bank = Addr >> CPU_BANK_SHIFT;
    uint32_t Addr21 = (CPU->MPR[Bank] << CPU_BANK_SHIFT) | (Addr & CPU_BANK_MASK);
    return Addr21;
}

uint8_t _MMU_Read(huc6280_state *CPU, huc6280_bus *Mem, uint16_t Addr)
{
    return Mem->Get(Mem->Instance, _MMU_Map(CPU, Addr));
}

void _MMU_Write(huc6280_state *CPU, huc6280_bus *Mem, uint16_t Addr, uint8_t Value)
{
    Mem->Set(Mem->Instance, _MMU_Map(CPU, Addr), Value);
}

void _MMU_TAM(huc6280_state *CPU, uint8_t Bitmap)
{
    for (int i = 0; i < 8; i++)
    {
        if (Bitmap & (1 << i)) CPU->MPR[i] = CPU->A;
    }
}

void _MMU_TMA(huc6280_state *CPU, uint8_t Bitmap)
{
    // No idea how this is combined
    for (int i = 0; i < 8; i++)
    {
        if (Bitmap & (1 << i)) CPU->A = CPU->MPR[i];
    }
}

huc6280_decoder HuC6280_MakeDecoder()
{
    huc6280_decoder Decoder;

    memset(Decoder.ITBL, 0xFF, sizeof(Decoder.ITBL));
    memset(Decoder.ATBL, 0xFF, sizeof(Decoder.ATBL));
    memset(Decoder.CTBL, 0x00, sizeof(Decoder.CTBL));

    for (int i = 0; i < sizeof(HUC6280_OPC_DECODE_INFO)/sizeof(HUC6280_OPC_DECODE_INFO[0]); i++)
    {
        huc6280_opc_mapping Mapping = HUC6280_OPC_DECODE_INFO[i];

        Decoder.ITBL[Mapping.Opcode] = Mapping.Instr;
        Decoder.ATBL[Mapping.Opcode] = Mapping.AddrMode;
        Decoder.CTBL[Mapping.Opcode] = Mapping.Cycles;
    }

    return Decoder;
}

const char *HuC6280_Instr_Name(huc6280_instr Instr)
{
    if (Instr >= INSTRUCTION_COUNT)
    {
        return "???";
    }
    else
    {
        return HUC6280_INSTR_TO_STR[Instr];
    }
}

huc6280_instr_decoded HuC6280_Decode_Instr(huc6280_decoder *Decoder, huc6280_state *CPU, huc6280_bus *Mem, uint16_t Addr)
{
    huc6280_instr_decoded Decoded = { 0 };

    uint8_t OpCode = _MMU_Read(CPU, Mem, Addr);
    Decoded.Id = Decoder->ITBL[OpCode];
    Decoded.AddrMode = Decoder->ATBL[OpCode];
    Decoded.Cycles = Decoder->CTBL[OpCode];

    if (Decoded.Id == 0xFF)
    {
        printf("Invalid opcode: %02X\n", OpCode);
        assert(false && "Invalid instruction");
    }

    size_t OperandSize = 0;

    switch (Decoded.AddrMode)
    {
        // TODO: 6 for Special format 2
        case AM_IMP: OperandSize = 0; break;
        case AM_IMM: OperandSize = 1; break;
        case AM_ZPG: OperandSize = 1; break;
        case AM_ZPX: OperandSize = 1; break;
        case AM_ZPY: OperandSize = 1; break;
        case AM_ZPR: OperandSize = 2; break;
        case AM_ZPI: OperandSize = 1; break;
        case AM_ZXI: OperandSize = 1; break;
        case AM_ZIY: OperandSize = 1; break;
        case AM_ABS: OperandSize = 2; break;
        case AM_ABX: OperandSize = 2; break;
        case AM_ABY: OperandSize = 2; break;
        case AM_ABI: OperandSize = 2; break;
        case AM_AXI: OperandSize = 2; break;
        case AM_REL: OperandSize = 1; break;
        case AM_IZP: OperandSize = 2; break;
        case AM_IZX: OperandSize = 2; break;
        case AM_IAB: OperandSize = 3; break;
        case AM_IAX: OperandSize = 3; break;
        case AM_ACC: OperandSize = 0; break;
    }

    switch (Decoded.Id)
    {
        case TAM: OperandSize = 1; break;
        case TMA: OperandSize = 1; break;
        default: break;
    }

    for (size_t i = 0; i < OperandSize; i++)
    {
        Decoded.Operand[i] = _MMU_Read(CPU, Mem, Addr+1+i);
    }

    Decoded.Size = OperandSize + 1;

    return Decoded;
}

void HuC6280_Format_Decoded_Instr(char *Dest, huc6280_instr_decoded Decoded)
{
    int i = 0;
    i += sprintf(Dest, "%s ", HuC6280_Instr_Name(Decoded.Id));

    switch (Decoded.Id)
    {
        case TAM: sprintf(Dest+i, "#$%02X", Decoded.Operand[0]); return;
        case TMA: sprintf(Dest+i, "#$%02X", Decoded.Operand[0]); return;
        default: break;
    }

    switch (Decoded.AddrMode)
    {
        // TODO: Operand for Special format 2
        case AM_IMP: break;
        case AM_IMM: sprintf(Dest+i, "#$%02X", Decoded.Operand[0]); break;
        case AM_ZPG: sprintf(Dest+i, "$%02X", Decoded.Operand[0]); break;
        case AM_ZPX: sprintf(Dest+i, "$%02X,X", Decoded.Operand[0]); break;
        case AM_ZPY: sprintf(Dest+i, "$%02X,Y", Decoded.Operand[0]); break;
        case AM_ZPR: sprintf(Dest+i, "$%02X,%+03i", Decoded.Operand[0], Decoded.Operand[1]); break;
        case AM_ZPI: sprintf(Dest+i, "($%02X)", Decoded.Operand[0]); break;
        case AM_ZXI: sprintf(Dest+i, "($%02X,X)", Decoded.Operand[0]); break;
        case AM_ZIY: sprintf(Dest+i, "($%02X),Y", Decoded.Operand[0]); break;
        case AM_ABS: sprintf(Dest+i, "$%02X%02X", Decoded.Operand[1], Decoded.Operand[0]); break;
        case AM_ABX: sprintf(Dest+i, "$%02X%02X,X", Decoded.Operand[1], Decoded.Operand[0]); break;
        case AM_ABY: sprintf(Dest+i, "$%02X%02X,Y", Decoded.Operand[1], Decoded.Operand[0]); break;
        case AM_ABI: sprintf(Dest+i, "($%02X%02X)", Decoded.Operand[1], Decoded.Operand[0]); break;
        case AM_AXI: sprintf(Dest+i, "($%02X%02X,X)", Decoded.Operand[1], Decoded.Operand[0]); break;
        case AM_REL: sprintf(Dest+i, "%+03i", Decoded.Operand[0]); break;
        case AM_IZP: sprintf(Dest+i, "#$%02X $%02X", Decoded.Operand[0], Decoded.Operand[1]); break;
        case AM_IZX: sprintf(Dest+i, "#$%02X $%02X,X", Decoded.Operand[0], Decoded.Operand[1]); break;
        case AM_IAB: sprintf(Dest+i, "#$%02X $%02X%02X", Decoded.Operand[0], Decoded.Operand[2], Decoded.Operand[1]); break;
        case AM_IAX: sprintf(Dest+i, "#$%02X $%02X%02X,X", Decoded.Operand[0], Decoded.Operand[2], Decoded.Operand[1]); break;
        case AM_ACC: sprintf(Dest+i, "A"); break;
    }
}

void HuC6280_PowerUp(huc6280_state *CPU)
{
    *CPU = (huc6280_state){ 0 };
    _SetFlag(CPU, FLAG_INT, 1);
    _SetFlag(CPU, FLAG_BRK, 1);
    CPU->PC = CPU_BANK_SEL(7);
}

void HuC6280_Run_Instr(huc6280_state *CPU, huc6280_instr_decoded Instr, huc6280_bus *Mem)
{
    if (CPU->IsCrashed)
    {
        return;
    }
    
    CPU->PC += Instr.Size;
    if (CPU->IsFast)
    {
        CPU->Cycles += Instr.Cycles;
    }
    else
    {
        CPU->Cycles += Instr.Cycles * 4;
    }

    switch (Instr.Id)
    {
        case CLD:
            _SetFlag(CPU, FLAG_DEC, 0);
            _SetFlag(CPU, FLAG_MEM, 0);
            break;
        case CSH:
            CPU->IsFast = true;
            _SetFlag(CPU, FLAG_MEM, 0); // Just an assumption
            break;
        case LDX:
            CPU->X = Instr.Operand[0];
            _SetFlag(CPU, FLAG_MEM, 0);
            _SetFlag(CPU, FLAG_ZER, CPU->X == 0);
            _SetFlag(CPU, FLAG_NEG, CPU->X & 0x80);
            break;
        case LDA:
            CPU->A = Instr.Operand[0];
            _SetFlag(CPU, FLAG_MEM, 0);
            _SetFlag(CPU, FLAG_ZER, CPU->A == 0);
            _SetFlag(CPU, FLAG_NEG, CPU->A & 0x80);
            break;
        case SEI:
            _SetFlag(CPU, FLAG_INT, 1);
            _SetFlag(CPU, FLAG_MEM, 0);
            break;
        case STZ:
            _MMU_Write(CPU, Mem, Instr.Operand[0], 0);
            _SetFlag(CPU, FLAG_MEM, 0);
            break;
        case TAM:
            _MMU_TAM(CPU, Instr.Operand[0]);
            _SetFlag(CPU, FLAG_MEM, 0);
            break;
        case TMA:
            _MMU_TMA(CPU, Instr.Operand[0]);
            _SetFlag(CPU, FLAG_MEM, 0);
            break;
        case TXS:
            CPU->SP = CPU->X;
            _SetFlag(CPU, FLAG_MEM, 0);
            break;
        default:
            printf("Unimplemented instruction: %s\n", HuC6280_Instr_Name(Instr.Id));
            CPU->IsCrashed = true;
            break;
    }
}

void HuC6280_Next_Instr(huc6280_state *CPU, huc6280_decoder *Decoder, huc6280_bus *Mem)
{
    huc6280_instr_decoded Instr = HuC6280_Decode_Instr(Decoder, CPU, Mem, CPU->PC);
    char InstrStr[100];
    HuC6280_Format_Decoded_Instr(InstrStr, Instr);
    printf("%s\n", InstrStr);
    HuC6280_Run_Instr(CPU, Instr, Mem);
}
