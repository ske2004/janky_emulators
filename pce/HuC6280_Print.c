#include "HuC6280_Internal.c"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

const char *HUC6280_INSTR_TO_STR[] =
{
    "ADC","AND","ASL",
    "BBR0","BBR1","BBR2","BBR3","BBR4","BBR5","BBR6","BBR7",
    "BCC","BCS","BEQ","BIT","BMI","BNE","BPL","BRA","BRK",
    "BBS0","BBS1","BBS2","BBS3","BBS4","BBS5","BBS6","BBS7",
    "BVC","BVS",
    "CLC","CLD","CLI","CLV","CMP","CPX","CPY","CSH",
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
    "TAI","TAM","TAX","TAY","TDD","TIA","TII","TIN",
    "TMA","TRB","TSB","TSX","TXA","TXS","TYA",
    "WAI",
    "???"
};

const size_t HUC6280_INSTR_TO_STR_LEN = sizeof(HUC6280_INSTR_TO_STR)/sizeof(HUC6280_INSTR_TO_STR[0]);

const char *HUC6280_ADR_MODE_TO_STR[] =
{
    "IMP", 
    "#NN", 
    "ZP", 
    "ZP,X", 
    "ZP,Y", 
    "ZP,REL", 
    "(ZP)", 
    "(ZP,X)", 
    "(ZP),Y", 
    "$NNNN", 
    "$NNNN,X", 
    "$NNNN,Y", 
    "($NNNN)", 
    "($NNNN,X)", 
    "REL", 
    "IMM ZP", 
    "IMM ZP,X",
    "IMM ABS",
    "IMM ABS,X",
    "A"
};

const size_t HUC6280_ADR_MODE_TO_STR_LEN = sizeof(HUC6280_ADR_MODE_TO_STR)/sizeof(HUC6280_ADR_MODE_TO_STR[0]);
const char *HUC6280_REG_TO_STR[] =
{
    "A", "X", "Y", "SP"
};

const size_t HUC6280_REG_TO_STR_LEN = sizeof(HUC6280_REG_TO_STR)/sizeof(HUC6280_REG_TO_STR[0]);

const char *HUC6280_FLAG_TO_STR[] =
{
    "C", "Z", "I", "D", "B", "T", "V", "N"
};

const size_t HUC6280_FLAG_TO_STR_LEN = sizeof(HUC6280_FLAG_TO_STR)/sizeof(HUC6280_FLAG_TO_STR[0]);

const char *HUC6280_MICRO_OP_REG_TO_STR[] =
{
    "PT1", "PT2", "LEN", "TM1", "TM2"
};

const size_t HUC6280_MICRO_OP_REG_TO_STR_LEN = sizeof(HUC6280_MICRO_OP_REG_TO_STR)/sizeof(HUC6280_MICRO_OP_REG_TO_STR[0]);

const char *HUC6280_MICRO_OP_TYPE_TO_STR[] =
{
    "NOP",           
    "READ_NEXT",     
    "READ_NEXT_LO",  
    "READ_NEXT_HI",  
    "READ_NEXT_DMY", 
    "SET_FLAG",      
    "CLEAR_FLAG",    
    "ARITH_FLAGS",   
    "GET_CPU_REG",   
    "SET_CPU_REG",   

    "DEREF_LO",
    "DEREF_HI",
    "READ_MEM", 
    "WRITE_MEM", 

    "GOTO_LZ",       
    "GOTO_LNZ",

    "INDEX_X_ZPG",
    "INDEX_X",
    "INDEX_Y",

    "REG_INC",
    "REG_DEC",

    "OP_OR",
    "OP_AND",
    "OP_XOR",
    "OP_ADD",
    "OP_SUB",

    "STACK_READ",
    "STACK_WRITE",
    "STACK_INC",
    "STACK_DEC",

    // Ad hoc
    "EXEC_TAM",
    "EXEC_STZ",
    "EXEC_CSH",
};

const size_t HUC6280_MICRO_OP_TYPE_TO_STR_LEN = sizeof(HUC6280_MICRO_OP_TYPE_TO_STR)/sizeof(HUC6280_MICRO_OP_TYPE_TO_STR[0]);

void HuC6280_Print_Micro_Op(micro_op Op)
{
    micro_op_type Type = (micro_op_type)(Op & 0xFF);
    uint32_t Operand = (Op >> 8) & 0xFF;

    assert(Type < HUC6280_MICRO_OP_TYPE_TO_STR_LEN);

    printf("%20s ", HUC6280_MICRO_OP_TYPE_TO_STR[Type]);

    switch (Type)
    {
        // Reg printer
        case MO_READ_NEXT: 
        case MO_READ_NEXT_LO: 
        case MO_READ_NEXT_HI: 
        case MO_READ_MEM: 
        case MO_WRITE_MEM:
        case MO_DEREF_LO:
        case MO_DEREF_HI:
        case MO_INDEX_X_ZPG:
        case MO_INDEX_X:
        case MO_INDEX_Y:
            printf("(%-7s) ", HUC6280_MICRO_OP_REG_TO_STR[Operand]);
            break;

        // Flag printer
        case MO_SET_FLAG: 
        case MO_CLEAR_FLAG: 
            printf("%-10s", HUC6280_FLAG_TO_STR[Operand]);
            break;

        // CPU reg printer
        case MO_GET_CPU_REG: 
        case MO_SET_CPU_REG:
            printf("%-10s", HUC6280_REG_TO_STR[Operand]);
            break;

        default: 
            printf("%-9c ", ' ');
            break;
    }
}

void HuC6280_Print_Micro_Ops(huc6280_micro_ops *MicroOps)
{
    int Cycles = 1;

    for (size_t i = 0; i < MicroOps->Count; i++)
    {
        Cycles += HUC6280_MICRO_OP_CYCLES[MicroOps->Ops[i] & 0xFF];
        printf("    \x1b[33m% 3d \x1b[31m", Cycles);
        HuC6280_Print_Micro_Op(MicroOps->Ops[i]);
        printf("\x1b[0m\n");
    }
}

void HuC6280_Dump_Known_Ops(huc6280_decoder *Decoder)
{
    for (size_t i = 0; i < 256; i++)
    {
        if (Decoder->ITBL[i] == 0xFF) continue;
        printf("%02X: %s %s (Cycles: %d)\n", (int)i, HUC6280_INSTR_TO_STR[Decoder->ITBL[i]], HUC6280_ADR_MODE_TO_STR[Decoder->ATBL[i]], Decoder->CTBL[i]);
        HuC6280_Print_Micro_Ops(&Decoder->MicroOps[i]);
    }
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
        case TAI: OperandSize = 6; break;
        case TDD: OperandSize = 6; break;
        case TIA: OperandSize = 6; break;
        case TII: OperandSize = 6; break;
        case TIN: OperandSize = 6; break;
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
        case TMA:
        case TAM: sprintf(Dest+i, "#$%02X", Decoded.Operand[0]); return;
        case TAI:
        case TDD:
        case TIA:
        case TII:
        case TIN:
            sprintf(Dest+i, "SRC: $%02X%02X DST: $%02X%02X LEN: $%02X%02X", Decoded.Operand[1], Decoded.Operand[0], Decoded.Operand[3], Decoded.Operand[2], Decoded.Operand[5], Decoded.Operand[4]);
            return;
        default: break;
    }

    switch (Decoded.AddrMode)
    {
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

