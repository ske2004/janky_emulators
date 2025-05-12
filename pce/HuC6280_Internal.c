#ifndef HUC6280_INTERNAL_H
#define HUC6280_INTERNAL_H

#include "HuC6280.h"

// 16 bites - 13 bits = 3 bits = 8 banks
#define CPU_BANK_SHIFT 13
#define CPU_BANK_SIZE 0x2000
// 0x2000 - 1 :3
#define CPU_BANK_MASK 0x1FFF
#define CPU_BANK_SEL(Addr) ((Addr) << CPU_BANK_SHIFT)

typedef enum
{
    REG_A,
    REG_X,
    REG_Y,
    REG_SP,
} huc6280_reg;

typedef enum 
{
    REG_PT1, // pointer 1
    REG_PT2, // pointer 2
    REG_LEN, // length
    REG_TMP, // temporary
} micro_op_reg;

typedef enum 
{
    MO_NOP,           // Waste 1 cycle
    MO_READ_NEXT,     // 0 -> REG; [PC++] -> REG_LO
    MO_READ_NEXT_LO,  // [PC++] -> REG_LO
    MO_READ_NEXT_HI,  // [PC++] -> REG_HI
    MO_READ_NEXT_DMY, // [PC++] -> NUL
    MO_SET_FLAG,      // Operand -> Flag
    MO_CLEAR_FLAG,    // Operand -> Flag
    MO_GET_CPU_REG,   // CPU REG -> Operand
    MO_SET_CPU_REG,   // Operand -> CPU REG

    // Memory
    MO_READ_MEM,      // 0 -> TMP; [reg] -> TMP
    MO_WRITE_MEM,     // 0 -> TMP; TMP -> [reg]

    // Ad hoc
    MO_EXEC_TAM,
    MO_EXEC_STZ,
    MO_EXEC_CSH,
} micro_op_type;

typedef struct
{
    uint8_t Opcode;
    huc6280_addr_mode AddrMode;
    huc6280_instr Instr;
    uint8_t Cycles;
} huc6280_opc_mapping;

const huc6280_opc_mapping HUC6280_OPC_DECODE_INFO[] =
{
    {0x53, AM_IMP, TAM, 5},

    {0x64, AM_ZPG, STZ, 4},

    {0x78, AM_IMP, SEI, 2},

    {0x9A, AM_IMP, TXS, 2},
    
    {0xA2, AM_IMM, LDX, 2},
    {0xA9, AM_IMM, LDA, 2},

    {0xD4, AM_IMP, CSH, 3},
    {0xD8, AM_IMP, CLD, 2},
};

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

void _SetFlag(huc6280_state *CPU, huc6280_flags Flag, bool Value)
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

bool _GetFlag(huc6280_state *CPU, huc6280_flags Flag)
{
    return CPU->Flags & (1 << Flag);
}

uint8_t _GetRegister(huc6280_state *CPU, huc6280_reg Reg)
{
    switch (Reg)
    {
        case REG_A: return CPU->A;
        case REG_X: return CPU->X;
        case REG_Y: return CPU->Y;
        case REG_SP: return CPU->SP;
    }
}

void _SetRegister(huc6280_state *CPU, huc6280_reg Reg, uint8_t Value)
{
    switch (Reg)
    {
        case REG_A: CPU->A = Value; break;
        case REG_X: CPU->X = Value; break;
        case REG_Y: CPU->Y = Value; break;
        case REG_SP: CPU->SP = Value; break;
    }
}


#endif