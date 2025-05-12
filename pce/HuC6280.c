// This CPU is modified version of 65C02 by WDC.
// I based this on ricoh.c from neske.

#include "HuC6280.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
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
        case AM_IMM: break;
        case AM_ZPG: 
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
        default:
            assert(false && "TODO (emit write)");
            break;
    }
}

huc6280_micro_ops _InstrToMicroOps(huc6280_decoder *Decoder, uint8_t OpCode)
{
    size_t Count = 0;
    huc6280_micro_ops MicroOps = { 0 };

    huc6280_addr_mode AddrMode = Decoder->ATBL[OpCode];
    huc6280_instr Instr = Decoder->ITBL[OpCode];

    // TODO: Special cases for TAM, TMA, TAI, TDD, TIA, TII, TIN
    switch (AddrMode)
    {
        case AM_ACC:
        case AM_IMP:
            if (Instr != TAM && Instr != TMA)
                _Emit_I(&MicroOps, MO_READ_NEXT_DMY);
            break;
        case AM_IMM:
            _Emit_IV(&MicroOps, MO_READ_NEXT, REG_TMP);
            break;
        case AM_ZPG:
            _Emit_IV(&MicroOps, MO_READ_NEXT, REG_PT1);
            break;
        default:
            printf("AddrMode: %d\n", AddrMode);
            assert(false && "TODO (addr mode)");
            break;
    }

    switch (Instr)
    {
        case TAM:
            _Emit_IV(&MicroOps, MO_READ_NEXT, REG_TMP);
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
            break;
        case LDA:
            _Emit_Read(&MicroOps, AddrMode);
            _Emit_IV(&MicroOps, MO_SET_CPU_REG, REG_A);
            break;
        case CSH:
            _Emit_I(&MicroOps, MO_EXEC_CSH);
            break;
        case CLD:
            _Emit_IV(&MicroOps, MO_CLEAR_FLAG, FLAG_DEC);
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

void HuC6280_PowerUp(huc6280_state *CPU)
{
    *CPU = (huc6280_state){ 0 };
    _SetFlag(CPU, FLAG_INT, 1);
    _SetFlag(CPU, FLAG_BRK, 1);
    CPU->PC = CPU_BANK_SEL(7);
}

void HuC6280_Next_Cycle(huc6280_state *CPU, huc6280_decoder *Decoder, huc6280_bus *Mem)
{

}
