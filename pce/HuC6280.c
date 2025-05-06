// This CPU is modified version of 65C02 by WDC.
// I based this on ricoh.c from neske.

#include "HuC6280.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    bool IsInvalid;
    bool IsImm;
    bool IsAcc;
    uint16_t Addr;
    uint8_t Imm;
} huc6280_address;

const char *HUC6280_INSTR_TO_STR[] =
{
    "ADC", "AND", "ASL",
    "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", "BPL", "BRK", "BVC", "BVS",
    "CLC", "CLD", "CLI", "CLV", "CMP", "CPX", "CPY",
    "DEC", "DEX", "DEY",
    "EOR",
    "INC", "INX", "INY",
    "JMP", "JSR",
    "LDA", "LDX", "LDY", "LSR",
    "NOP",
    "ORA",
    "PHA", "PHP", "PLA", "PLP",
    "ROL", "ROR", "RTI", "RTS",
    "SBC", "SEC", "SED", "SEI", "STA", "STX", "STY",
    "TAX", "TAY", "TSX", "TXA", "TXS", "TYA", "???"
};

// 0xFF - Invalid addressing mode
// Addressing modes in this order:
//      Acc  Abs  AbX  AbY  Imm  Imp  Ind  Xnd  InY  Rel  Zpg  ZpX  ZpY
const uint8_t HUC6280_OPC_TO_INSTR[] =
{
/*ADC*/ 0xFF,0x6D,0x7D,0x79,0x69,0xFF,0xFF,0x61,0x71,0xFF,0x65,0x75,0xFF,
/*AND*/ 0xFF,0x2D,0x3D,0x39,0x29,0xFF,0xFF,0x21,0x31,0xFF,0x25,0x35,0xFF,
/*ASL*/ 0x0A,0x0E,0x1E,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x06,0x16,0xFF,
/*BCC*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x90,0xFF,0xFF,0xFF,
/*BCS*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xB0,0xFF,0xFF,0xFF,
/*BEQ*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF0,0xFF,0xFF,0xFF,
/*BIT*/ 0xFF,0x2C,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x24,0xFF,0xFF,
/*BMI*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x30,0xFF,0xFF,0xFF,
/*BNE*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xD0,0xFF,0xFF,0xFF,
/*BPL*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x10,0xFF,0xFF,0xFF,
/*BRK*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*BVC*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x50,0xFF,0xFF,0xFF,
/*BVS*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x70,0xFF,0xFF,0xFF,
/*CLC*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x18,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*CLD*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xD8,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*CLI*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x58,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*CLV*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xB8,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*CMP*/ 0xFF,0xCD,0xDD,0xD9,0xC9,0xFF,0xFF,0xC1,0xD1,0xFF,0xC5,0xD5,0xFF,
/*CPX*/ 0xFF,0xEC,0xFF,0xFF,0xE0,0xFF,0xFF,0xFF,0xFF,0xFF,0xE4,0xFF,0xFF,
/*CPY*/ 0xFF,0xCC,0xFF,0xFF,0xC0,0xFF,0xFF,0xFF,0xFF,0xFF,0xC4,0xFF,0xFF,
/*DEC*/ 0xFF,0xCE,0xDE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xC6,0xD6,0xFF,
/*DEX*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xCA,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*DEY*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x88,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*EOR*/ 0xFF,0x4D,0x5D,0x59,0x49,0xFF,0xFF,0x41,0x51,0xFF,0x45,0x55,0xFF,
/*INC*/ 0xFF,0xEE,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xE6,0xF6,0xFF,
/*INX*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xE8,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*INY*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xC8,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*JMP*/ 0xFF,0x4C,0xFF,0xFF,0xFF,0xFF,0x6C,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*JSR*/ 0xFF,0x20,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*LDA*/ 0xFF,0xAD,0xBD,0xB9,0xA9,0xFF,0xFF,0xA1,0xB1,0xFF,0xA5,0xB5,0xFF,
/*LDX*/ 0xFF,0xAE,0xFF,0xBE,0xA2,0xFF,0xFF,0xFF,0xFF,0xFF,0xA6,0xFF,0xB6,
/*LDY*/ 0xFF,0xAC,0xBC,0xFF,0xA0,0xFF,0xFF,0xFF,0xFF,0xFF,0xA4,0xB4,0xFF,
/*LSR*/ 0x4A,0x4E,0x5E,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x46,0x56,0xFF,
/*NOP*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xEA,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*ORA*/ 0xFF,0x0D,0x1D,0x19,0x09,0xFF,0xFF,0x01,0x11,0xFF,0x05,0x15,0xFF,
/*PHA*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x48,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*PHP*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x08,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*PLA*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x68,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*PLP*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x28,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*ROL*/ 0x2A,0x2E,0x3E,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x26,0x36,0xFF,
/*ROR*/ 0x6A,0x6E,0x7E,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x66,0x76,0xFF,
/*RTI*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x40,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*RTS*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x60,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*SBC*/ 0xFF,0xED,0xFD,0xF9,0xE9,0xFF,0xFF,0xE1,0xF1,0xFF,0xE5,0xF5,0xFF,
/*SEC*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x38,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*SED*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xF8,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*SEI*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x78,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*STA*/ 0xFF,0x8D,0x9D,0x99,0xFF,0xFF,0xFF,0x81,0x91,0xFF,0x85,0x95,0xFF,
/*STX*/ 0xFF,0x8E,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x86,0xFF,0x96,
/*STY*/ 0xFF,0x8C,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x84,0x94,0xFF,
/*TAX*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xAA,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*TAY*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xA8,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*TSX*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0xBA,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*TXA*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x8A,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*TXS*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x9A,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
/*TYA*/ 0xFF,0xFF,0xFF,0xFF,0xFF,0x98,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
};  

// 0 - Invalid
// Addressing modes in this order:
//      Acc Abs AbX AbY Imm Imp Ind Xnd InY Rel Zpg ZpX ZpY
const uint8_t HUC6280_CYCLE_TBL[] =
{
/*ADC*/ 0,  4,  4,  4,  2,  0,  0,  6,  5,  0,  3,  4,  0,  
/*AND*/ 0,  4,  4,  4,  2,  0,  0,  6,  5,  0,  3,  4,  0,  
/*ASL*/ 2,  6,  7,  0,  0,  0,  0,  0,  0,  0,  5,  6,  0,  
/*BCC*/ 0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  
/*BCS*/ 0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  
/*BEQ*/ 0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  
/*BIT*/ 0,  4,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  
/*BMI*/ 0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  
/*BNE*/ 0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  
/*BPL*/ 0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  
/*BRK*/ 0,  0,  0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  
/*BVC*/ 0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  
/*BVS*/ 0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  
/*CLC*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*CLD*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*CLI*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*CLV*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*CMP*/ 0,  4,  4,  4,  2,  0,  0,  6,  5,  0,  3,  4,  0,  
/*CPX*/ 0,  4,  0,  0,  2,  0,  0,  0,  0,  0,  3,  0,  0,  
/*CPY*/ 0,  4,  0,  0,  2,  0,  0,  0,  0,  0,  3,  0,  0,  
/*DEC*/ 0,  6,  7,  0,  0,  0,  0,  0,  0,  0,  5,  6,  0,  
/*DEX*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*DEY*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*EOR*/ 0,  4,  4,  4,  2,  0,  0,  6,  5,  0,  3,  4,  0,  
/*INC*/ 0,  6,  7,  0,  0,  0,  0,  0,  0,  0,  5,  6,  0,  
/*INX*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*INY*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*JMP*/ 0,  3,  0,  0,  0,  0,  5,  0,  0,  0,  0,  0,  0,  
/*JSR*/ 0,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
/*LDA*/ 0,  4,  4,  4,  2,  0,  0,  6,  5,  0,  3,  4,  0,  
/*LDX*/ 0,  4,  0,  4,  2,  0,  0,  0,  0,  0,  3,  0,  4,  
/*LDY*/ 0,  4,  4,  0,  2,  0,  0,  0,  0,  0,  3,  4,  0,  
/*LSR*/ 2,  6,  7,  0,  0,  0,  0,  0,  0,  0,  5,  6,  0,  
/*NOP*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*ORA*/ 0,  4,  4,  4,  2,  0,  0,  6,  5,  0,  3,  4,  0,  
/*PHA*/ 0,  0,  0,  0,  0,  3,  0,  0,  0,  0,  0,  0,  0,  
/*PHP*/ 0,  0,  0,  0,  0,  3,  0,  0,  0,  0,  0,  0,  0,  
/*PLA*/ 0,  0,  0,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0,  
/*PLP*/ 0,  0,  0,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0,  
/*ROL*/ 2,  6,  7,  0,  0,  0,  0,  0,  0,  0,  5,  6,  0,  
/*ROR*/ 2,  6,  7,  0,  0,  0,  0,  0,  0,  0,  5,  6,  0,  
/*RTI*/ 0,  0,  0,  0,  0,  6,  0,  0,  0,  0,  0,  0,  0,  
/*RTS*/ 0,  0,  0,  0,  0,  6,  0,  0,  0,  0,  0,  0,  0,  
/*SBC*/ 0,  4,  4,  4,  2,  0,  0,  6,  5,  0,  3,  4,  0,  
/*SEC*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*SED*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*SEI*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*STA*/ 0,  4,  5,  5,  0,  0,  0,  6,  6,  0,  3,  4,  0,  
/*STX*/ 0,  4,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  4,  
/*STY*/ 0,  4,  0,  0,  0,  0,  0,  0,  0,  0,  3,  4,  0,  
/*TAX*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*TAY*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*TSX*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*TXA*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*TXS*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
/*TYA*/ 0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0,  
};


huc6280_decoder HuC6280_MakeDecoder()
{
    huc6280_decoder Decoder;

    memset(Decoder.ITBL, 0xFF, sizeof(Decoder.ITBL));
    memset(Decoder.ATBL, 0xFF, sizeof(Decoder.ATBL));

    for (int i = 0; i < ADDR_MODE_COUNT*_ICOUNT; i++)
    {
        if (HUC6280_OPC_TO_INSTR[i] != 0xFF)
        {
            Decoder.ITBL[HUC6280_OPC_TO_INSTR[i]] = i/ADDR_MODE_COUNT;
            Decoder.ATBL[HUC6280_OPC_TO_INSTR[i]] = i%ADDR_MODE_COUNT;
        }
    }

    return Decoder;
}

const char *HuC6280_Instr_Name(huc6280_instr Instr)
{
    if (Instr >= _ICOUNT)
    {
        return "???";
    }
    else
    {
        return HUC6280_INSTR_TO_STR[Instr];
    }
}

huc6280_instr_decoded HuC6280_Decode_Instr(huc6280_decoder *Decoder, huc6280_bus *Mem, uint16_t Addr)
{
    huc6280_instr_decoded Decoded = { 0 };
    uint8_t opc = Mem->Get(Mem->Instance, Addr);
    Decoded.Id = Decoder->ITBL[opc];
    Decoded.AddrMode = Decoder->ATBL[opc];

    if (Decoded.Id == 0xFF)
    {
        Decoded.Id = _ICOUNT;
        Decoded.AddrMode = AM_IMP;
        Decoded.Size = 1;
        return Decoded;
    }

    size_t OperandSize = 0;

    switch (Decoded.AddrMode)
    {
        case AM_ACC: break;
        case AM_ABS: OperandSize = 2; break;
        case AM_ABX: OperandSize = 2; break;
        case AM_ABY: OperandSize = 2; break;
        case AM_IMM: OperandSize = 1; break;
        case AM_IMP: if (Decoded.Id == BRK) OperandSize = 1; break;
        case AM_IND: OperandSize = 2; break;
        case AM_XND: OperandSize = 1; break;
        case AM_INY: OperandSize = 1; break;
        case AM_REL: OperandSize = 1; break;
        case AM_ZPG: OperandSize = 1; break;
        case AM_ZPX: OperandSize = 1; break;
        case AM_ZPY: OperandSize = 1; break;
    }

    for (size_t i = 0; i < OperandSize; i++)
    {
        Decoded.Operand[i] = Mem->Get(Mem->Instance, Addr+1+i);
    }

    Decoded.Size = OperandSize + 1;

    return Decoded;
}

void HuC6280_Format_Decoded_Instr(char *Dest, huc6280_instr_decoded Decoded)
{
    int i = 0;
    i += sprintf(Dest, "%s ", HuC6280_Instr_Name(Decoded.Id));

    switch (Decoded.AddrMode)
    {
        case AM_ACC: sprintf(Dest+i, "A"); break;
        case AM_ABS: sprintf(Dest+i, "$%04X", *(uint16_t*)Decoded.Operand); break;
        case AM_ABX: sprintf(Dest+i, "$%04X,X", *(uint16_t*)Decoded.Operand); break;
        case AM_ABY: sprintf(Dest+i, "$%04X,Y", *(uint16_t*)Decoded.Operand); break;
        case AM_IMM: sprintf(Dest+i, "#$%02X", Decoded.Operand[0]); break;
        case AM_IMP: break;
        case AM_IND: sprintf(Dest+i, "($%04X)", *(uint16_t*)Decoded.Operand); break;
        case AM_XND: sprintf(Dest+i, "($%02X,X)", Decoded.Operand[0]); break;
        case AM_INY: sprintf(Dest+i, "($%02X),Y", Decoded.Operand[0]); break;
        case AM_REL: sprintf(Dest+i, "$%02X", Decoded.Operand[0]); break;
        case AM_ZPG: sprintf(Dest+i, "$%02X", Decoded.Operand[0]); break;
        case AM_ZPX: sprintf(Dest+i, "$%02X,X", Decoded.Operand[0]); break;
        case AM_ZPY: sprintf(Dest+i, "$%02X,Y", Decoded.Operand[0]); break;
    }
}

static uint8_t _Read8(huc6280_state *CPU, huc6280_bus *Mem, uint16_t Addr)
{
    return Mem->Get(Mem->Instance, Addr);
}

static void _Write8(huc6280_state *CPU, huc6280_bus *Mem, uint16_t Addr, uint8_t Val)
{
    Mem->Set(Mem->Instance, Addr, Val);
}

static uint16_t _Read16(huc6280_state *CPU, huc6280_bus *Mem, uint16_t Addr)
{
    Addr = (uint16_t)_Read8(CPU, Mem, Addr) |
           ((uint16_t)_Read8(CPU, Mem, Addr+1) << 8);


    return Addr;
}

static uint16_t _Read16zp(huc6280_state *CPU, huc6280_bus *Mem, uint16_t Addr)
{
    Addr = (uint16_t)_Read8(CPU, Mem, (Addr)&0xFF) |
           ((uint16_t)_Read8(CPU, Mem, (Addr+1)&0xFF) << 8);
    
    return Addr;
}

uint16_t _ReadJMP(
    huc6280_state *CPU,
    huc6280_instr_decoded Instr,
    huc6280_bus *Mem
)
{
    fflush(stdout);
    switch (Instr.AddrMode)
    {
        case AM_ABS: return *(uint16_t*)Instr.Operand;
        case AM_IND: 
            // indirect jump page wraparound bug
            if (Instr.Operand[0] == 0xFF)
            {
                return (uint16_t)_Read8(CPU, Mem, *(uint16_t*)Instr.Operand) |
                       ((uint16_t)_Read8(CPU, Mem, *(uint16_t*)Instr.Operand-0xFF) << 8);
            }
            else
            {
                return _Read16(CPU, Mem,  *(uint16_t*)Instr.Operand); 
            }
        default: assert(false && "oh no");
    }

    return 0; 
}

static void _SetFlag(huc6280_state *CPU, huc6280_flags Flag, bool State)
{
    CPU->Flags = (CPU->Flags & ~(1<<Flag)) | (State<<Flag);
}

static bool _GetFlag(huc6280_state *CPU, huc6280_flags Flag)
{
    return (CPU->Flags & (1<<Flag)) > 0;
}

static void _Push8(huc6280_state *CPU, huc6280_bus *Mem, uint8_t Val)
{
    _Write8(CPU, Mem, CPU->SP + 0x100, Val);
    CPU->SP -= 1;
}

static void push16(huc6280_state *CPU, huc6280_bus *Mem, uint16_t Val)
{
    _Push8(CPU, Mem, (Val>>8)&0xFF);
    _Push8(CPU, Mem, Val&0xFF);
}

static uint8_t _Pull8(huc6280_state *CPU, huc6280_bus *Mem)
{
    CPU->SP += 1;
    return _Read8(CPU, Mem, CPU->SP + 0x100);
}

static uint16_t _Pull16(huc6280_state *CPU, huc6280_bus *Mem)
{
    CPU->SP += 2;
    return (_Read8(CPU, Mem, (CPU->SP - 0) + 0x100) << 8) |
            _Read8(CPU, Mem, (CPU->SP - 1) + 0x100);
}


#define REG_A 0
#define REG_X 1
#define REG_Y 2

static int8_t _UpdateFlags(huc6280_state *CPU, int8_t Value)
{
    _SetFlag(CPU, FLAG_NEG, Value < 0);
    _SetFlag(CPU, FLAG_ZER, Value == 0);

    return Value;
}

static void _SetReg(huc6280_state *CPU, int Reg, int8_t Value)
{
    switch (Reg)
    {
        case REG_A: CPU->A = _UpdateFlags(CPU, Value); break;
        case REG_X: CPU->X = _UpdateFlags(CPU, Value); break;
        case REG_Y: CPU->Y = _UpdateFlags(CPU, Value); break;
    }
}

static uint16_t _PageCross(huc6280_state *CPU, uint16_t A, uint16_t B)
{
    if (((A + B) >> 8) != (A >> 8))
    {
        CPU->Cycles++;
    }

    return A + B;
}

static uint16_t _PageCrossBranch(huc6280_state *CPU, uint16_t A, int8_t B)
{
    if (((A + B) >> 8) != (A >> 8))
    {
        CPU->Cycles += 2;
    }
    else
    {
        CPU->Cycles += 1;
    }

    return A + B;
}

huc6280_address _MakeAddress(
    huc6280_state *CPU,
    huc6280_instr_decoded Instr,
    huc6280_bus *Mem
)
{
    huc6280_address Addr = { 0 };
    
    switch (Instr.AddrMode)
    {
        case AM_ACC: Addr.IsAcc = true; break;
        case AM_ABS: Addr.Addr = *(uint16_t*)Instr.Operand; break;
        case AM_ABX: Addr.Addr = _PageCross(CPU, *(uint16_t*)Instr.Operand, CPU->X); break;
        case AM_ABY: Addr.Addr = _PageCross(CPU, *(uint16_t*)Instr.Operand, CPU->Y); break;
        case AM_IMM: Addr.IsImm = true; Addr.Imm = Instr.Operand[0]; break;
        case AM_IMP: Addr.IsInvalid = true; break;
        case AM_IND: Addr.Addr = _Read16(CPU, Mem, *(uint16_t*)Instr.Operand); break;
        case AM_XND: Addr.Addr = _Read16zp(CPU, Mem, (uint8_t)(Instr.Operand[0] + CPU->X)); break;
        case AM_INY: Addr.Addr = _PageCross(CPU, _Read16zp(CPU, Mem, Instr.Operand[0]), CPU->Y); break;
        case AM_REL: Addr.IsInvalid = true; break;
        case AM_ZPG: Addr.Addr = Instr.Operand[0]; break;
        case AM_ZPX: Addr.Addr = (uint8_t)(Instr.Operand[0] + CPU->X); break;
        case AM_ZPY: Addr.Addr = (uint8_t)(Instr.Operand[0] + CPU->Y); break;
    }

    return Addr;
}

static uint8_t _DoRead(huc6280_state *CPU, huc6280_address Addr, huc6280_bus *Mem)
{
    assert(Addr.IsInvalid == false && "i fucked up oops");

    if (Addr.IsAcc)
    {
        return CPU->A;
    }
    else if (Addr.IsImm)
    {
        return Addr.Imm;
    }
    else
    {
        return _Read8(CPU, Mem, Addr.Addr);
    }
}

static void _DoWrite(huc6280_state *CPU, huc6280_address Addr, huc6280_bus *Mem, uint8_t Val)
{
    assert(Addr.IsInvalid == false && "i fucked up oops");
    assert(Addr.IsImm == false && "imm write >_>");

    if (Addr.IsAcc)
    {
        _SetReg(CPU, REG_A, Val);
    }
    else
    {
        _Write8(CPU, Mem, Addr.Addr, Val);
    }
}

int8_t _DoAddCarry(
    huc6280_state *CPU,
    uint8_t A, uint8_t B
)
{
    uint8_t Carry = _GetFlag(CPU, FLAG_CAR);
    int8_t Result = (int8_t)A + (int8_t)B + (int8_t)Carry;
    uint16_t CarryBits = (uint16_t)A + (uint16_t)B + (uint16_t)Carry;
    _SetFlag(CPU, FLAG_NEG, Result < 0);
    _SetFlag(CPU, FLAG_ZER, Result == 0);
    _SetFlag(CPU, FLAG_CAR, (CarryBits&0xFF00) > 0);
    int16_t ovfw = (int16_t)(int8_t)A + (int16_t)(int8_t)B + (int16_t)Carry;
    _SetFlag(CPU, FLAG_OFW, ovfw < -128 || ovfw > 127);
    return Result;
}

int8_t _DoSubCarry(
    huc6280_state *CPU,
    uint8_t A, uint8_t B,
    bool Overflow,
    bool Carry
)
{
    uint8_t _Carry = (!_GetFlag(CPU, FLAG_CAR)) && Carry;
    int8_t Result = (int8_t)A - (int8_t)B - (int8_t)_Carry;
    uint16_t CarryBits = (uint16_t)A - (uint16_t)B - (uint16_t)_Carry;
    _SetFlag(CPU, FLAG_NEG, Result < 0);
    _SetFlag(CPU, FLAG_ZER, Result == 0);
    _SetFlag(CPU, FLAG_CAR, (CarryBits&0xFF00) == 0);
    if (Overflow) {
        int16_t ovfw = (int16_t)(int8_t)A - (int16_t)(int8_t)B - (int16_t)_Carry;
        _SetFlag(CPU, FLAG_OFW, ovfw < -128 || ovfw > 127);
    }
    return Result;
}

static void _DoRelJMP(huc6280_state *CPU, huc6280_instr_decoded Instr, bool Is)
{
    if (Is) {
        CPU->PC = _PageCrossBranch(CPU, CPU->PC, Instr.Operand[0]);
    }
}

static void _DoCMP(huc6280_state *CPU, int8_t Reg, int8_t Operand)
{
    _DoSubCarry(CPU, Reg, Operand, false, false);
}

void HuC6280_Do_Interrupt(
    huc6280_state *CPU,
    huc6280_bus *Mem,
    uint16_t NewPC
)
{
    push16(CPU, Mem, CPU->PC);
    _Push8(CPU, Mem, CPU->Flags | (1 << FLAG_BRK) | (1 << FLAG_BI5));
    CPU->PC = NewPC;
    CPU->Cycles += 7;
}

void HuC6280_Run_Instr(
    huc6280_state *CPU,
    huc6280_instr_decoded Instr,
    huc6280_bus *Mem
)
{
    uint8_t RMW_Temp = 0;
    size_t Start = CPU->PC;

    CPU->PC += Instr.Size;
    if (Instr.Id != _ICOUNT)
    {
        CPU->Cycles += HUC6280_CYCLE_TBL[Instr.AddrMode+Instr.Id*ADDR_MODE_COUNT];
    }

    huc6280_address Addr = _MakeAddress(CPU, Instr, Mem);

    switch (Instr.Id)
    {
        case ADC:
            _SetReg(CPU, REG_A, _DoAddCarry(CPU, CPU->A, _DoRead(CPU, Addr, Mem)));
            break;
        case AND:
            _SetReg(CPU, REG_A, CPU->A & _DoRead(CPU, Addr, Mem));
            break;
        case ASL:
            RMW_Temp = _DoRead(CPU, Addr, Mem);
            _DoWrite(CPU, Addr, Mem, RMW_Temp);
            _DoWrite(CPU, Addr, Mem, _UpdateFlags(CPU, RMW_Temp<<1));
            _SetFlag(CPU, FLAG_CAR, (RMW_Temp&0x80) > 0);
            break;
        case BCC:
            _DoRelJMP(CPU, Instr, _GetFlag(CPU, FLAG_CAR) == false);
            break;
        case BCS:
            _DoRelJMP(CPU, Instr, _GetFlag(CPU, FLAG_CAR) == true);
            break;
        case BEQ:
            _DoRelJMP(CPU, Instr, _GetFlag(CPU, FLAG_ZER) == true);
            break;
        case BIT:
            {
                uint8_t Byte = _DoRead(CPU, Addr, Mem);
                _SetFlag(CPU, FLAG_NEG, Byte>>7&1);
                _SetFlag(CPU, FLAG_OFW, Byte>>6&1);
                _SetFlag(CPU, FLAG_ZER, (Byte & CPU->A) == 0);
            }
            break;
        case BMI:
            _DoRelJMP(CPU, Instr, _GetFlag(CPU, FLAG_NEG) == true);
            break;
        case BNE:
            _DoRelJMP(CPU, Instr, _GetFlag(CPU, FLAG_ZER) == false);
            break;
        case BPL:
            _DoRelJMP(CPU, Instr, _GetFlag(CPU, FLAG_NEG) == false);
            break;
        case BRK:
            {
                // printf("OPCODE: %02X\n", mem->get(mem->instance, start));
                // printf("REGISTER DUMP: A=%02X X=%02X Y=%02X F=%02X P=%04X\n", (unsigned int)cpu->a, (unsigned int)cpu->x, (unsigned int)cpu->y, (unsigned int)cpu->flags, (unsigned int)cpu->pc);
                fflush(stdout);
                uint16_t PC = _Read16(CPU, Mem, 0xFFFE);
                push16(CPU, Mem, CPU->PC);
                _Push8(CPU, Mem, CPU->Flags | (1 << FLAG_BRK));
                CPU->PC = PC;
            }
            break;
        case BVC:
            _DoRelJMP(CPU, Instr, _GetFlag(CPU, FLAG_OFW) == false);
            break;
        case BVS:
            _DoRelJMP(CPU, Instr, _GetFlag(CPU, FLAG_OFW) == true);
            break;
        case CLC:
            _SetFlag(CPU, FLAG_CAR, false);
            break;
        case CLD:
            _SetFlag(CPU, FLAG_DEC, false);
            break;
        case CLI:
            _SetFlag(CPU, FLAG_INT, false);
            break;
        case CLV:
            _SetFlag(CPU, FLAG_OFW, false);
            break;
        case CMP:
            _DoCMP(CPU, CPU->A, _DoRead(CPU, Addr, Mem));
            break;
        case CPX:
            _DoCMP(CPU, CPU->X, _DoRead(CPU, Addr, Mem));
            break;
        case CPY:
            _DoCMP(CPU, CPU->Y, _DoRead(CPU, Addr, Mem));
            break;
        case DEC:
            RMW_Temp = _DoRead(CPU, Addr, Mem);
            _DoWrite(CPU, Addr, Mem, RMW_Temp);
            _DoWrite(CPU, Addr, Mem, _UpdateFlags(CPU, RMW_Temp-1));
            break;
        case DEX:
            _SetReg(CPU, REG_X, CPU->X-1);
            break;
        case DEY:
            _SetReg(CPU, REG_Y, CPU->Y-1);
            break;
        case EOR:
            _SetReg(CPU, REG_A, CPU->A ^ _DoRead(CPU, Addr, Mem));
            break;
        case INC:
            RMW_Temp = _DoRead(CPU, Addr, Mem);
            _DoWrite(CPU, Addr, Mem, RMW_Temp);
            _DoWrite(CPU, Addr, Mem, _UpdateFlags(CPU, RMW_Temp+1));
            break;
        case INX:
            _SetReg(CPU, REG_X, CPU->X+1);
            break;
        case INY:
            _SetReg(CPU, REG_Y, CPU->Y+1);
            break;
        case JMP:
            CPU->PC = _ReadJMP(CPU, Instr, Mem);
            break;
        case JSR:
            push16(CPU, Mem, CPU->PC-1);
            CPU->PC = _ReadJMP(CPU, Instr, Mem);
            break;
        case LDA:
            _SetReg(CPU, REG_A, _DoRead(CPU, Addr, Mem));
            break;
        case LDX:
            _SetReg(CPU, REG_X, _DoRead(CPU, Addr, Mem));
            break;
        case LDY:
            _SetReg(CPU, REG_Y, _DoRead(CPU, Addr, Mem));
            break;
        case LSR:
            RMW_Temp = _DoRead(CPU, Addr, Mem);
            _DoWrite(CPU, Addr, Mem, RMW_Temp);
            _DoWrite(CPU, Addr, Mem, _UpdateFlags(CPU, RMW_Temp>>1));
            _SetFlag(CPU, FLAG_CAR, (RMW_Temp&1) > 0);
            break;
        case NOP:
            break;
        case ORA:
            _SetReg(CPU, REG_A, CPU->A | _DoRead(CPU, Addr, Mem));
            break;
        case PHA:
            _Push8(CPU, Mem, CPU->A);
            break;
        case PHP:
            _Push8(CPU, Mem, CPU->Flags | (1 << FLAG_BRK) | (1 << FLAG_BI5));
            break;
        case PLA:
            _SetReg(CPU, REG_A, _Pull8(CPU, Mem));
            break;
        case PLP:
            CPU->Flags = (CPU->Flags & ((1 << FLAG_BRK) | (1 << FLAG_BI5))) | (_Pull8(CPU, Mem) & (((1 << FLAG_BRK) | (1 << FLAG_BI5)) ^ 0xFF));
            break;
        case ROL:
            RMW_Temp = _DoRead(CPU, Addr, Mem);
            _DoWrite(CPU, Addr, Mem, RMW_Temp);
            _DoWrite(CPU, Addr, Mem, _UpdateFlags(CPU, (RMW_Temp<<1)|_GetFlag(CPU, FLAG_CAR)));
            _SetFlag(CPU, FLAG_CAR, (RMW_Temp&0x80) > 0);
            break;
        case ROR:
            RMW_Temp = _DoRead(CPU, Addr, Mem);
            _DoWrite(CPU, Addr, Mem, RMW_Temp);
            _DoWrite(CPU, Addr, Mem, _UpdateFlags(CPU, (RMW_Temp>>1)|(_GetFlag(CPU, FLAG_CAR)<<7)));
            _SetFlag(CPU, FLAG_CAR, (RMW_Temp&0x1) > 0);
            break;
        case RTI:
            CPU->Flags = (CPU->Flags & ((1 << FLAG_BRK) | (1 << FLAG_BI5))) | (_Pull8(CPU, Mem) & (((1 << FLAG_BRK) | (1 << FLAG_BI5)) ^ 0xFF));
            CPU->PC = _Pull16(CPU, Mem);
            break;
        case RTS:
            CPU->PC = _Pull16(CPU, Mem)+1;
            break;
        case SBC:
            _SetReg(CPU, REG_A, _DoSubCarry(CPU, CPU->A, _DoRead(CPU, Addr, Mem), true, true));
            break;
        case SEC:
            _SetFlag(CPU, FLAG_CAR, 1);
            break;
        case SED:
            _SetFlag(CPU, FLAG_DEC, 1);
            break;
        case SEI:
            _SetFlag(CPU, FLAG_INT, 1);
            break;
        case STA:
            _DoWrite(CPU, Addr, Mem, CPU->A);
            break;
        case STX:
            _DoWrite(CPU, Addr, Mem, CPU->X);
            break;
        case STY:
            _DoWrite(CPU, Addr, Mem, CPU->Y);
            break;
        case TAX:
            _SetReg(CPU, REG_X, CPU->A);
            break;
        case TAY:
            _SetReg(CPU, REG_Y, CPU->A);
            break;
        case TSX:
            _SetReg(CPU, REG_X, CPU->SP);
            break;
        case TXA:
            _SetReg(CPU, REG_A, CPU->X);
            break;
        case TXS:
            CPU->SP = CPU->X;
            break;
        case TYA:
            _SetReg(CPU, REG_A, CPU->Y);
            CPU->A = CPU->Y;
            break;
        case _ICOUNT:
            printf("OPCODE: %02X\n", Mem->Get(Mem->Instance, Start));
            CPU->IsCrashed = 1;
            break;
    }
}
