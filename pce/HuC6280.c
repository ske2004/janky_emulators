// This CPU is modified version of 65C02 by WDC.
// I based this on ricoh.c from neske.

#include "HuC6280.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "HuC6280_Internal.c"
#include "HuC6280_Print.c" 

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

void _Emit_I(huc6280_micro_ops *MicroOps, micro_op_type Type)
{
    MicroOps->Ops[MicroOps->Count++] = Type;
}

void _Emit_IV(huc6280_micro_ops *MicroOps, micro_op_type Type, uint8_t Value)
{
    MicroOps->Ops[MicroOps->Count++] = Type | (Value << 8);
}

void _Emit_Read(huc6280_micro_ops *MicroOps, huc6280_addr_mode AddrMode)
{
    switch (AddrMode)
    {
        case AM_IMP: break;
        case AM_IMM: 
            _Emit_IV(MicroOps, MO_READ_NEXT, REG_TM1);
            break;
        case AM_ZPG: 
            _Emit_IV(MicroOps, MO_READ_MEM, REG_PT1);
            break;
        case AM_ZXI:
            _Emit_IV(MicroOps, MO_READ_MEM, REG_PT1);
            _Emit_IV(MicroOps, MO_READ_MEM, REG_PT1);
            break;
        default:
            assert(false && "TODO (emit read)");
            break;
    }
}

void _Emit_Write(huc6280_micro_ops *MicroOps, huc6280_addr_mode AddrMode)
{
    switch (AddrMode)
    {
        case AM_IMP: assert(false && "Illegal!"); break;
        case AM_IMM: assert(false && "Illegal!"); break;
        case AM_ZPG:
            _Emit_IV(MicroOps, MO_WRITE_MEM, REG_PT1);
            break;
        case AM_ZXI:
            _Emit_IV(MicroOps, MO_WRITE_MEM, REG_PT1);
            break;
        default:
            assert(false && "TODO (emit write)");
            break;
    }
}

uint32_t _Emit_L(huc6280_micro_ops *MicroOps, micro_op_type Type)
{
    uint32_t Loc = MicroOps->Count;
    _Emit_I(MicroOps, Type);
    return Loc;
}

huc6280_micro_ops _InstrToMicroOps(huc6280_decoder *Decoder, uint8_t OpCode)
{
    huc6280_micro_ops MicroOps = { 0 };

    huc6280_addr_mode AddrMode = Decoder->ATBL[OpCode];
    huc6280_instr Instr = Decoder->ITBL[OpCode];

    // TODO: Special cases for TAM, TMA, TAI, TDD, TIA, TII, TIN
    switch (AddrMode)
    {
        case AM_ACC:
        case AM_IMP:
            if (Instr != TAM && Instr != TMA && Instr != TIA)
                _Emit_I(&MicroOps, MO_READ_NEXT_DMY);
            break;
        case AM_IMM:
            _Emit_IV(&MicroOps, MO_READ_NEXT, REG_TM1);
            break;
        case AM_ZPG:
            _Emit_IV(&MicroOps, MO_READ_NEXT, REG_PT1);
            break;
        case AM_ZXI:
            _Emit_IV(&MicroOps, MO_READ_NEXT, REG_PT1);
            _Emit_IV(&MicroOps, MO_DEREF_LO, REG_PT1);
            _Emit_IV(&MicroOps, MO_DEREF_HI, REG_PT1);
            _Emit_IV(&MicroOps, MO_INDEX_X_ZPG, REG_PT1);
            break;
        default:
            printf("AddrMode: %d\n", AddrMode);
            assert(false && "TODO (addr mode)");
            break;
    }

    bool IsBlockInstruction = Instr == TIA;

    switch (Instr)
    {
        case TIA:
            _Emit_IV(&MicroOps, MO_READ_NEXT_LO, REG_PT1);
            _Emit_IV(&MicroOps, MO_READ_NEXT_HI, REG_PT1);
            _Emit_IV(&MicroOps, MO_READ_NEXT_LO, REG_PT2);
            _Emit_IV(&MicroOps, MO_READ_NEXT_HI, REG_PT2);
            _Emit_IV(&MicroOps, MO_READ_NEXT_LO, REG_LEN);
            _Emit_IV(&MicroOps, MO_READ_NEXT_HI, REG_LEN);
            _Emit_IV(&MicroOps, MO_GET_CPU_REG, REG_Y);
            _Emit_I(&MicroOps, MO_STACK_WRITE);
            _Emit_I(&MicroOps, MO_STACK_DEC);
            _Emit_IV(&MicroOps, MO_GET_CPU_REG, REG_A);
            _Emit_I(&MicroOps, MO_STACK_WRITE);
            _Emit_I(&MicroOps, MO_STACK_DEC);
            _Emit_IV(&MicroOps, MO_GET_CPU_REG, REG_X);
            _Emit_I(&MicroOps, MO_STACK_WRITE);

            _Emit_IV(&MicroOps, MO_READ_MEM, REG_PT1);
            _Emit_IV(&MicroOps, MO_WRITE_MEM, REG_PT2);
            _Emit_IV(&MicroOps, MO_REG_DEC, REG_LEN);
            _Emit_IV(&MicroOps, MO_REG_DEC, REG_PT1);
            _Emit_IV(&MicroOps, MO_REG_INC, REG_PT2);
            _Emit_IV(&MicroOps, MO_GOTO_LZ, 6);
            _Emit_IV(&MicroOps, MO_READ_MEM, REG_PT1);
            _Emit_IV(&MicroOps, MO_WRITE_MEM, REG_PT2);
            _Emit_IV(&MicroOps, MO_REG_DEC, REG_LEN);
            _Emit_IV(&MicroOps, MO_REG_INC, REG_PT1);
            _Emit_IV(&MicroOps, MO_REG_INC, REG_PT2);
            _Emit_IV(&MicroOps, MO_GOTO_LNZ, -11);

            _Emit_I(&MicroOps, MO_STACK_READ);
            _Emit_IV(&MicroOps, MO_SET_CPU_REG, REG_X);
            _Emit_I(&MicroOps, MO_STACK_INC);
            _Emit_I(&MicroOps, MO_STACK_READ);
            _Emit_IV(&MicroOps, MO_SET_CPU_REG, REG_A);
            _Emit_I(&MicroOps, MO_STACK_INC);
            _Emit_I(&MicroOps, MO_STACK_READ);
            _Emit_IV(&MicroOps, MO_SET_CPU_REG, REG_Y);

            break;
        case TAM:
            _Emit_IV(&MicroOps, MO_READ_NEXT, REG_TM1);
            _Emit_I(&MicroOps, MO_EXEC_TAM);
            break;
        case STZ:
            _Emit_IV(&MicroOps, MO_WRITE_MEM, REG_PT1);
            break;
        case SEI:
            _Emit_IV(&MicroOps, MO_SET_FLAG, FLAG_INT);
            break;
        case TXS:
            _Emit_IV(&MicroOps, MO_GET_CPU_REG, REG_X);
            _Emit_IV(&MicroOps, MO_SET_CPU_REG, REG_SP);
            break;
        case LDX:
            _Emit_Read(&MicroOps, AddrMode);
            _Emit_IV(&MicroOps, MO_SET_CPU_REG, REG_X);
            _Emit_IV(&MicroOps, MO_ARITH_FLAGS, REG_X);
            break;
        case LDA:
            _Emit_Read(&MicroOps, AddrMode);
            _Emit_IV(&MicroOps, MO_SET_CPU_REG, REG_A);
            _Emit_IV(&MicroOps, MO_ARITH_FLAGS, REG_A);
            break;
        case CSH:
            _Emit_I(&MicroOps, MO_EXEC_CSH);
            break;
        case CLD:
            _Emit_IV(&MicroOps, MO_CLEAR_FLAG, FLAG_DEC);
            break;
        case ORA:
            _Emit_Read(&MicroOps, AddrMode);
            _Emit_I(&MicroOps, MO_OP_OR);
            break;
        default:
            printf("Instr: %s\n", HUC6280_INSTR_TO_STR[Instr]);
            assert(false && "TODO (instr)");
            break;
    }

    // TODO: Check if this occurs after every instruction
    _Emit_IV(&MicroOps, MO_CLEAR_FLAG, FLAG_MEM);

    return MicroOps;
}

huc6280_decoder HuC6280_MakeDecoder()
{
    huc6280_decoder Decoder;

    memset(Decoder.ITBL, 0xFF, sizeof(Decoder.ITBL));
    memset(Decoder.ATBL, 0xFF, sizeof(Decoder.ATBL));
    memset(Decoder.CTBL, 0x00, sizeof(Decoder.CTBL));
    memset(Decoder.MicroOps, 0x00, sizeof(Decoder.MicroOps));

    for (int i = 0; i < sizeof(HUC6280_OPC_DECODE_INFO)/sizeof(HUC6280_OPC_DECODE_INFO[0]); i++)
    {
        huc6280_opc_mapping Mapping = HUC6280_OPC_DECODE_INFO[i];

        Decoder.ITBL[Mapping.Opcode] = Mapping.Instr;
        Decoder.ATBL[Mapping.Opcode] = Mapping.AddrMode;
        Decoder.CTBL[Mapping.Opcode] = Mapping.Cycles;
    }

    for (int i = 0; i < 256; i++)
    {
        if (Decoder.ITBL[i] == 0xFF) continue;
        Decoder.MicroOps[i] = _InstrToMicroOps(&Decoder, i);
    }

    return Decoder;
}

bool _RunMicroOp(huc6280_state *CPU, huc6280_bus *Mem)
{
    uint32_t MicroOp = CPU->MicroOps.Ops[CPU->MicroOps.Index];

    if (CPU->MicroOps.Index >= CPU->MicroOps.Count)
    {
        return true;
    }

    micro_op_type Type = (micro_op_type)(MicroOp & 0xFF);
    uint32_t Operand = (MicroOp >> 8) & 0xFF;

    CPU->MicroOps.Index++;

    HuC6280_Print_Micro_Op(MicroOp);
    printf("PC = %04x, Cycles = %llu\n", CPU->PC, CPU->Cycles);

    switch (Type)
    {
        case MO_NOP:
            return true;
        case MO_READ_NEXT:
            _SetInternalReg(CPU, Operand, _MMU_Read(CPU, Mem, CPU->PC), 0xFFFF);
            CPU->PC++;
            return true;
        case MO_READ_NEXT_LO:
            _SetInternalRegLo(CPU, Operand, _MMU_Read(CPU, Mem, CPU->PC));
            CPU->PC++;
            return true;
        case MO_READ_NEXT_HI:
            _SetInternalRegHi(CPU, Operand, _MMU_Read(CPU, Mem, CPU->PC));
            CPU->PC++;
            return true;
        case MO_READ_NEXT_DMY:
            return true;
        case MO_SET_FLAG:
            _SetFlag(CPU, Operand, 1);
            return false;
        case MO_CLEAR_FLAG:
            _SetFlag(CPU, Operand, 0);
            return false;
        case MO_ARITH_FLAGS:
            _SetArithFlags(CPU, Operand);
            return false;
        case MO_GET_CPU_REG:
            _SetInternalReg(CPU, Operand, _GetRegister(CPU, Operand), 0xFFFF);
            return false;
        case MO_SET_CPU_REG:
            _SetRegister(CPU, Operand, _GetInternalReg(CPU, Operand));
            return false;
        case MO_DEREF_LO:
            _SetInternalRegLo(CPU, Operand, _MMU_Read(CPU, Mem, _GetInternalReg(CPU, Operand)));
            return true;
        case MO_DEREF_HI:
            _SetInternalRegHi(CPU, Operand, _MMU_Read(CPU, Mem, _GetInternalReg(CPU, Operand)+1));
            return true;
        case MO_READ_MEM:
            _SetInternalReg(CPU, Operand, _MMU_Read(CPU, Mem, _GetInternalReg(CPU, Operand)), 0xFFFF);
            return true;
        case MO_WRITE_MEM:
            _MMU_Write(CPU, Mem, _GetInternalReg(CPU, Operand), _GetInternalReg(CPU, Operand));
            return true;
        case MO_GOTO_LZ:
            if (_GetInternalReg(CPU, REG_LEN) == 0)
                CPU->MicroOps.Index += (int8_t)Operand;
            return false;
        case MO_GOTO_LNZ:
            if (_GetInternalReg(CPU, REG_LEN) != 0)
                CPU->MicroOps.Index += (int8_t)Operand;
            return false;
        case MO_INDEX_X_ZPG:
            _SetInternalRegLo(CPU, Operand, (_GetInternalReg(CPU, Operand)&0xFF) + _GetRegister(CPU, REG_X));
            return true;
        case MO_INDEX_X:
            _SetInternalReg(CPU, Operand, _GetInternalReg(CPU, Operand) + _GetRegister(CPU, REG_X), 0xFFFF);
            return true;
        case MO_INDEX_Y:
            _SetInternalReg(CPU, Operand, _GetInternalReg(CPU, Operand) + _GetRegister(CPU, REG_Y), 0xFFFF);
            return true;
        case MO_REG_INC:
            _SetInternalReg(CPU, Operand, _GetInternalReg(CPU, Operand) + 1, 0xFFFF);
            return true;
        case MO_REG_DEC:
            _SetInternalReg(CPU, Operand, _GetInternalReg(CPU, Operand) - 1, 0xFFFF);
            return true;
        case MO_STACK_READ:
            _MMU_Write(CPU, Mem, _GetRegister(CPU, REG_SP)+0x100, _GetInternalReg(CPU, REG_TM1));
            return true;
        case MO_STACK_WRITE:
            _SetInternalReg(CPU, REG_TM1, _MMU_Read(CPU, Mem, _GetRegister(CPU, REG_SP)+0x100), 0xFFFF);
            return true;
        case MO_STACK_INC:
            CPU->SP++;
            return false;
        case MO_STACK_DEC:
            CPU->SP--;
            return false;
        case MO_OP_OR:
            _SetInternalReg(CPU, REG_TM1, _GetInternalReg(CPU, REG_TM1) | _GetInternalReg(CPU, REG_TM2), 0xFFFF);
            return false;
        case MO_OP_AND:
            _SetInternalReg(CPU, REG_TM1, _GetInternalReg(CPU, REG_TM1) & _GetInternalReg(CPU, REG_TM2), 0xFFFF);
            return false;
        case MO_OP_XOR:
            _SetInternalReg(CPU, REG_TM1, _GetInternalReg(CPU, REG_TM1) ^ _GetInternalReg(CPU, REG_TM2), 0xFFFF);
            return false;
        case MO_OP_ADD:
            _SetInternalReg(CPU, REG_TM1, _GetInternalReg(CPU, REG_TM1) + _GetInternalReg(CPU, REG_TM2), 0xFFFF);
            return false;
        case MO_OP_SUB:
            _SetInternalReg(CPU, REG_TM1, _GetInternalReg(CPU, REG_TM1) - _GetInternalReg(CPU, REG_TM2), 0xFFFF);
            return false;
        case MO_EXEC_TAM:
            _MMU_TAM(CPU, _GetInternalReg(CPU, REG_TM1));
            return true;
        case MO_EXEC_STZ:
            _MMU_Write(CPU, Mem, _GetInternalReg(CPU, REG_PT1), 0x00);
            return true;
        case MO_EXEC_CSH:
            _MMU_Write(CPU, Mem, _GetInternalReg(CPU, REG_PT1), 0x00);
            return true;

    }
}

void HuC6280_PowerUp(huc6280_state *CPU)
{
    *CPU = (huc6280_state){ 0 };
    _SetFlag(CPU, FLAG_INT, 1);
    _SetFlag(CPU, FLAG_BRK, 1);
    CPU->PC = CPU_BANK_SEL(7);
}

void HuC6280_Next_Cycle(huc6280_state *CPU, huc6280_decoder *Decoder, huc6280_bus *Mem)
{
    if (CPU->Timer > 0)
    {
        CPU->Timer--;
        return;
    }

    if (CPU->MicroOps.Index >= CPU->MicroOps.Count)
    {
        uint8_t OpCode = _MMU_Read(CPU, Mem, CPU->PC);
        huc6280_instr Instr = Decoder->ITBL[OpCode];
        if (Instr == 0xFF)
        {
            printf("OpCode: %02x\n", OpCode);
            assert(false && "Unknown instruction");
        }

        printf("Instr: %02x %s\n", OpCode, HUC6280_INSTR_TO_STR[Instr]);
        CPU->MicroOps = Decoder->MicroOps[OpCode];
        if (CPU->MicroOps.Count == 0)
        {
            assert(false && "Unknown instruction");
        }

        CPU->PC++;
        CPU->Cycles++;
    }
    else 
    {
        while (!_RunMicroOp(CPU, Mem))
            ;
        if (CPU->MicroOps.Index < CPU->MicroOps.Count)
        {
            CPU->Cycles++;
        }
    }

    if (CPU->IsFast)
    {
        CPU->Timer = 0;
    }
    else
    {
        CPU->Timer = 3;
    }
}
