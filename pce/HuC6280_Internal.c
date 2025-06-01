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
    REG_TM1, // temporary
    REG_TM2, // temporary 2
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
    MO_ARITH_FLAGS,   // Sets arithmetic flags (accepts 1 parameter: which CPU register)
    MO_GET_CPU_REG,   // CPU REG -> Operand
    MO_SET_CPU_REG,   // Operand -> CPU REG

    // Memory
    MO_DEREF_LO,         
    MO_DEREF_HI,
    MO_READ_MEM,      // 0 -> TM1; [reg] -> TM1
    MO_WRITE_MEM,     // 0 -> TM1; TM1 -> [reg]

    MO_GOTO_LZ,       
    MO_GOTO_LNZ,

    MO_INDEX_X_ZPG,
    MO_INDEX_X,
    MO_INDEX_Y,

    MO_REG_INC,
    MO_REG_DEC,

    MO_OP_OR,         // TM1 | TM2 -> TM1
    MO_OP_AND,        // TM1 & TM2 -> TM1
    MO_OP_XOR,        // TM1 ^ TM2 -> TM1
    MO_OP_ADD,        // TM1 + TM2 -> TM1
    MO_OP_SUB,        // TM1 - TM2 -> TM1

    MO_STACK_READ,
    MO_STACK_WRITE,
    MO_STACK_INC,
    MO_STACK_DEC,

    // Ad hoc
    MO_EXEC_TAM,
    MO_EXEC_STZ,
    MO_EXEC_CSH,
} micro_op_type;

const size_t HUC6280_MICRO_OP_CYCLES[] =
{
    [MO_NOP] = 1,
    [MO_READ_NEXT] = 1,
    [MO_READ_NEXT_LO] = 1,
    [MO_READ_NEXT_HI] = 1,
    [MO_READ_NEXT_DMY] = 1,
    [MO_SET_FLAG] = 0,
    [MO_CLEAR_FLAG] = 0,
    [MO_ARITH_FLAGS] = 0,
    [MO_GET_CPU_REG] = 0,
    [MO_SET_CPU_REG] = 0,

    [MO_DEREF_LO] = 1,
    [MO_DEREF_HI] = 1,
    [MO_READ_MEM] = 1,
    [MO_WRITE_MEM] = 1,

    [MO_GOTO_LZ] = 0,
    [MO_GOTO_LNZ] = 0,

    [MO_INDEX_X_ZPG] = 0,
    [MO_INDEX_X] = 0,
    [MO_INDEX_Y] = 0,

    [MO_REG_INC] = 0,
    [MO_REG_DEC] = 0,

    [MO_OP_OR] = 0,
    [MO_OP_AND] = 0,
    [MO_OP_XOR] = 0,
    [MO_OP_ADD] = 0,
    [MO_OP_SUB] = 0,

    [MO_STACK_READ] = 1,
    [MO_STACK_WRITE] = 1,
    [MO_STACK_INC] = 0,
    [MO_STACK_DEC] = 0,

    [MO_EXEC_TAM] = 1,
    [MO_EXEC_STZ] = 1,
    [MO_EXEC_CSH] = 1,
};

typedef struct
{
    uint8_t Opcode;
    huc6280_addr_mode AddrMode;
    huc6280_instr Instr;
    uint8_t Cycles;
} huc6280_opc_mapping;

const huc6280_opc_mapping HUC6280_OPC_DECODE_INFO[] =
{
    {0x01, AM_ZXI, ORA, 7},
    {0x53, AM_IMP, TAM, 5},

    {0x64, AM_ZPG, STZ, 4},

    {0x78, AM_IMP, SEI, 2},

    {0x9A, AM_IMP, TXS, 2},
    
    {0xA2, AM_IMM, LDX, 2},
    {0xA9, AM_IMM, LDA, 2},

    {0xD4, AM_IMP, CSH, 3},
    {0xD8, AM_IMP, CLD, 2},
    
    {0xE3, AM_IMP, TIA, 17}
};

// Maps 16 bit address to 21 bit address
static inline uint32_t _MMU_Map(huc6280_state *CPU, uint16_t Addr)
{
    uint8_t Bank = Addr >> CPU_BANK_SHIFT;
    uint32_t Addr21 = (CPU->MPR[Bank] << CPU_BANK_SHIFT) | (Addr & CPU_BANK_MASK);
    return Addr21;
}

static inline uint8_t _MMU_Read(huc6280_state *CPU, huc6280_bus *Mem, uint16_t Addr)
{
    return Mem->Get(Mem->Instance, _MMU_Map(CPU, Addr));
}

static inline void _MMU_Write(huc6280_state *CPU, huc6280_bus *Mem, uint16_t Addr, uint8_t Value)
{
    Mem->Set(Mem->Instance, _MMU_Map(CPU, Addr), Value);
}

static inline void _SetFlag(huc6280_state *CPU, huc6280_flags Flag, bool Value)
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

static inline bool _GetFlag(huc6280_state *CPU, huc6280_flags Flag)
{
    return CPU->Flags & (1 << Flag);
}

static inline uint8_t _GetRegister(huc6280_state *CPU, huc6280_reg Reg)
{
    switch (Reg)
    {
        case REG_A: return CPU->A;
        case REG_X: return CPU->X;
        case REG_Y: return CPU->Y;
        case REG_SP: return CPU->SP;
    }
}

static inline void _SetRegister(huc6280_state *CPU, huc6280_reg Reg, uint8_t Value)
{
    switch (Reg)
    {
        case REG_A: CPU->A = Value; break;
        case REG_X: CPU->X = Value; break;
        case REG_Y: CPU->Y = Value; break;
        case REG_SP: CPU->SP = Value; break;
    }
}

static inline uint16_t _GetInternalReg(huc6280_state *CPU, micro_op_reg Reg)
{
    switch (Reg)
    {
        case REG_PT1: return CPU->MicroOps.Src;
        case REG_PT2: return CPU->MicroOps.Dst;
        case REG_LEN: return CPU->MicroOps.Len;
        case REG_TM1: return CPU->MicroOps.Tmp;
        case REG_TM2: return CPU->MicroOps.Tmp2;
    }
}

static inline void _SetInternalReg(huc6280_state *CPU, micro_op_reg Reg, uint16_t Value, uint16_t Mask)
{
    switch (Reg)
    {
        case REG_PT1: CPU->MicroOps.Src = (CPU->MicroOps.Src & ~Mask) | (Value & Mask); break;
        case REG_PT2: CPU->MicroOps.Dst = (CPU->MicroOps.Dst & ~Mask) | (Value & Mask); break;
        case REG_LEN: CPU->MicroOps.Len = (CPU->MicroOps.Len & ~Mask) | (Value & Mask); break;
        case REG_TM1: CPU->MicroOps.Tmp = (CPU->MicroOps.Tmp & ~Mask) | (Value & Mask); break;
        case REG_TM2: CPU->MicroOps.Tmp2 = (CPU->MicroOps.Tmp2 & ~Mask) | (Value & Mask); break;
    }
}

static inline void _SetInternalRegLo(huc6280_state *CPU, micro_op_reg Reg, uint8_t Value)
{
    _SetInternalReg(CPU, Reg, Value, 0xFF);
}

static inline void _SetInternalRegHi(huc6280_state *CPU, micro_op_reg Reg, uint8_t Value)
{
    _SetInternalReg(CPU, Reg, Value << 8, 0xFF00);
}

static inline void _SetArithFlags(huc6280_state *CPU, huc6280_reg Reg)
{
    uint8_t Value = _GetRegister(CPU, Reg);
    CPU->Flags = (CPU->Flags & ~FLAG_NEG) | ((Value & 0x80) ? FLAG_NEG : 0);
    CPU->Flags = (CPU->Flags & ~FLAG_ZER) | ((Value == 0) ? FLAG_ZER : 0);
}

#endif