#include "neske.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

const char *ricoh_instr_to_str[] =
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
    "TAX", "TAY", "TSX", "TXA", "TXS", "TYA", 
};

// 0xFF - Invalid addressing mode
// Addressing modes in this order:
//      Acc  Abs  AbX  AbY  Imm  Imp  Ind  Xnd  InY  Rel  Zpg  ZpX  ZpY
const uint8_t ricoh_opc_to_instr[] =
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
const uint8_t ricoh_cycle_tbl[] =
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


struct ricoh_decoder make_ricoh_decoder()
{
    struct ricoh_decoder decoder;

    memset(decoder.itbl, 0xFF, sizeof(decoder.itbl));
    memset(decoder.atbl, 0xFF, sizeof(decoder.atbl));

    for (int i = 0; i < ADDR_MODE_COUNT*_ICOUNT; i++)
    {
        if (ricoh_opc_to_instr[i] != 0xFF)
        {
            decoder.itbl[ricoh_opc_to_instr[i]] = i/ADDR_MODE_COUNT;
            decoder.atbl[ricoh_opc_to_instr[i]] = i%ADDR_MODE_COUNT;
        }
    }

    return decoder;
}

const char *ricoh_instr_name(enum instr instr)
{
    if (instr >= _ICOUNT)
    {
        return "???";
    }
    else
    {
        return ricoh_instr_to_str[instr];
    }
}

struct instr_decoded ricoh_decode_instr(struct ricoh_decoder *decoder, struct ricoh_mem_interface *mem, uint16_t addr)
{
    struct instr_decoded decoded = { 0 };
    uint8_t opc = mem->get(mem->instance, addr);
    decoded.id = decoder->itbl[opc];
    decoded.addr_mode = decoder->atbl[opc];

    size_t operand_size = 0;

    switch (decoded.addr_mode)
    {
        case AM_ACC: break;
        case AM_ABS: operand_size = 2; break;
        case AM_ABX: operand_size = 2; break;
        case AM_ABY: operand_size = 2; break;
        case AM_IMM: operand_size = 1; break;
        case AM_IMP: break;
        case AM_IND: operand_size = 2; break;
        case AM_XND: operand_size = 1; break;
        case AM_INY: operand_size = 1; break;
        case AM_REL: operand_size = 1; break;
        case AM_ZPG: operand_size = 1; break;
        case AM_ZPX: operand_size = 1; break;
        case AM_ZPY: operand_size = 1; break;
    }

    for (size_t i = 0; i < operand_size; i++)
    {
        decoded.operand[i] = mem->get(mem->instance, addr+1+i);
    }

    decoded.size = operand_size + 1;

    return decoded;
}

void ricoh_format_decoded_instr(char *dest, struct instr_decoded decoded)
{
    int i = 0;
    i += sprintf(dest, "%s ", ricoh_instr_name(decoded.id));

    switch (decoded.addr_mode)
    {
        case AM_ACC: sprintf(dest+i, "A"); break;
        case AM_ABS: sprintf(dest+i, "$%04X", *(uint16_t*)decoded.operand); break;
        case AM_ABX: sprintf(dest+i, "$%04X,X", *(uint16_t*)decoded.operand); break;
        case AM_ABY: sprintf(dest+i, "$%04X,Y", *(uint16_t*)decoded.operand); break;
        case AM_IMM: sprintf(dest+i, "#$%02X", decoded.operand[0]); break;
        case AM_IMP: break;
        case AM_IND: sprintf(dest+i, "($%04X)", *(uint16_t*)decoded.operand); break;
        case AM_XND: sprintf(dest+i, "($%02X,X)", decoded.operand[0]); break;
        case AM_INY: sprintf(dest+i, "($%02X),Y", decoded.operand[0]); break;
        case AM_REL: sprintf(dest+i, "$%02X", decoded.operand[0]); break;
        case AM_ZPG: sprintf(dest+i, "$%02X", decoded.operand[0]); break;
        case AM_ZPX: sprintf(dest+i, "$%02X,X", decoded.operand[0]); break;
        case AM_ZPY: sprintf(dest+i, "$%02X,Y", decoded.operand[0]); break;
    }
}

static uint8_t read_8(struct ricoh_state *cpu, struct ricoh_mem_interface *mem, uint16_t addr)
{
    return mem->get(mem->instance, addr);
}

static void write_8(struct ricoh_state *cpu, struct ricoh_mem_interface *mem, uint16_t addr, uint8_t val)
{
    mem->set(mem->instance, addr, val);
}

static uint16_t read_16(struct ricoh_state *cpu, struct ricoh_mem_interface *mem, uint16_t addr)
{
    addr = (uint16_t)read_8(cpu, mem, addr) |
           ((uint16_t)read_8(cpu, mem, addr+1) << 8);


    return addr;
}

static uint16_t read_16zp(struct ricoh_state *cpu, struct ricoh_mem_interface *mem, uint16_t addr)
{
    addr = (uint16_t)read_8(cpu, mem, (addr)&0xFF) |
           ((uint16_t)read_8(cpu, mem, (addr+1)&0xFF) << 8);
    
    return addr;
}

uint16_t do_readjmp(
    struct ricoh_state *cpu,
    struct instr_decoded instr,
    struct ricoh_mem_interface *mem
)
{
    fflush(stdout);
    switch (instr.addr_mode)
    {
        case AM_ABS: return *(uint16_t*)instr.operand;
        case AM_IND: 
            // indirect jump page wraparound bug
            if (instr.operand[0] == 0xFF)
            {
                return (uint16_t)read_8(cpu, mem, *(uint16_t*)instr.operand) |
                       ((uint16_t)read_8(cpu, mem, *(uint16_t*)instr.operand-0xFF) << 8);
            }
            else
            {
                return read_16(cpu, mem,  *(uint16_t*)instr.operand); 
            }
        default: assert(false && "oh no");
    }

    return 0; 
}

static void setflag(struct ricoh_state *cpu, enum flags flag, bool state)
{
    cpu->flags = (cpu->flags & ~(1<<flag)) | (state<<flag);
}

static bool getflag(struct ricoh_state *cpu, enum flags flag)
{
    return (cpu->flags & (1<<flag)) > 0;
}

static void push8(struct ricoh_state *cpu, struct ricoh_mem_interface *mem, uint8_t val)
{
    write_8(cpu, mem, cpu->sp + 0x100, val);
    cpu->sp -= 1;
}

static void push16(struct ricoh_state *cpu, struct ricoh_mem_interface *mem, uint16_t val)
{
    push8(cpu, mem, (val>>8)&0xFF);
    push8(cpu, mem, val&0xFF);
}

static uint8_t pull8(struct ricoh_state *cpu, struct ricoh_mem_interface *mem)
{
    cpu->sp += 1;
    return read_8(cpu, mem, cpu->sp + 0x100);
}

static uint16_t pull16(struct ricoh_state *cpu, struct ricoh_mem_interface *mem)
{
    cpu->sp += 2;
    return (read_8(cpu, mem, (cpu->sp - 0) + 0x100) << 8) |
            read_8(cpu, mem, (cpu->sp - 1) + 0x100);
}


#define REG_A 0
#define REG_X 1
#define REG_Y 2

static int8_t updateflags(struct ricoh_state *cpu, int8_t value)
{
    setflag(cpu, FLAG_NEG, value < 0);
    setflag(cpu, FLAG_ZER, value == 0);

    return value;
}

static void setreg(struct ricoh_state *cpu, int reg, int8_t value)
{
    switch (reg)
    {
        case REG_A: cpu->a = updateflags(cpu, value); break;
        case REG_X: cpu->x = updateflags(cpu, value); break;
        case REG_Y: cpu->y = updateflags(cpu, value); break;
    }
}

static uint16_t pagecross(struct ricoh_state *cpu, uint16_t a, uint16_t b)
{
    if (((a + b) >> 8) != (a >> 8))
    {
        cpu->cycles++;
    }

    return a + b;
}

static uint16_t pagecrossbranch(struct ricoh_state *cpu, uint16_t a, int8_t b)
{
    if (((a + b) >> 8) != (a >> 8))
    {
        cpu->cycles += 2;
    }
    else
    {
        cpu->cycles += 1;
    }

    return a + b;
}

uint8_t do_read(
    struct ricoh_state *cpu,
    struct instr_decoded instr,
    struct ricoh_mem_interface *mem
)
{
    switch (instr.addr_mode)
    {
        case AM_ACC: return cpu->a;
        case AM_ABS: return read_8(cpu, mem, *(uint16_t*)instr.operand);
        case AM_ABX: return read_8(cpu, mem, pagecross(cpu, *(uint16_t*)instr.operand, cpu->x));
        case AM_ABY: return read_8(cpu, mem, pagecross(cpu, *(uint16_t*)instr.operand, cpu->y));
        case AM_IMM: return instr.operand[0];
        case AM_IMP: assert(false && "Handle yourself");
        case AM_IND: return read_8(cpu, mem, read_16(cpu, mem, *(uint16_t*)instr.operand));
        case AM_XND: return read_8(cpu, mem, read_16zp(cpu, mem, (uint8_t)(instr.operand[0] + cpu->x)));
        case AM_INY: return read_8(cpu, mem, pagecross(cpu, read_16zp(cpu, mem, instr.operand[0]), cpu->y));
        case AM_REL: assert(false && "handle it in branches uwu");
        case AM_ZPG: return read_8(cpu, mem, instr.operand[0]);
        case AM_ZPX: return read_8(cpu, mem, (uint8_t)(instr.operand[0] + cpu->x));
        case AM_ZPY: return read_8(cpu, mem, (uint8_t)(instr.operand[0] + cpu->y));
    }

    return 0;
}


void do_write(
    struct ricoh_state *cpu,
    struct instr_decoded instr,
    struct ricoh_mem_interface *mem,
    uint8_t byte
)
{
    switch (instr.addr_mode)
    {
        case AM_ACC: setreg(cpu, REG_A, byte); break;
        case AM_ABS: write_8(cpu, mem, *(uint16_t*)instr.operand, byte); break;
        case AM_ABX: write_8(cpu, mem, pagecross(cpu, *(uint16_t*)instr.operand, cpu->x), byte); break;
        case AM_ABY: write_8(cpu, mem, pagecross(cpu, *(uint16_t*)instr.operand, cpu->y), byte); break;
        case AM_IMM: assert(false && "u wot m8"); break;
        case AM_IMP: assert(false && "void"); break;
        case AM_IND: write_8(cpu, mem, read_16(cpu, mem, *(uint16_t*)instr.operand), byte); break;
        case AM_XND: write_8(cpu, mem, read_16zp(cpu, mem, (uint8_t)(instr.operand[0] + cpu->x)), byte); break;
        case AM_INY: write_8(cpu, mem, pagecross(cpu, read_16zp(cpu, mem, instr.operand[0]), cpu->y), byte); break;
        case AM_REL: assert(false && "doesn't even make sense"); break;
        case AM_ZPG: write_8(cpu, mem, instr.operand[0], byte); break;
        case AM_ZPX: write_8(cpu, mem, (uint8_t)(instr.operand[0] + cpu->x), byte); break;
        case AM_ZPY: write_8(cpu, mem, (uint8_t)(instr.operand[0] + cpu->y), byte); break;
    }
}

int8_t do_add_carry(
    struct ricoh_state *cpu,
    uint8_t a, uint8_t b
)
{
    uint8_t car = getflag(cpu, FLAG_CAR);
    int8_t res = (int8_t)a + (int8_t)b + (int8_t)car;
    uint16_t carrybits = (uint16_t)a + (uint16_t)b + (uint16_t)car;
    setflag(cpu, FLAG_NEG, res < 0);
    setflag(cpu, FLAG_ZER, res == 0);
    setflag(cpu, FLAG_CAR, (carrybits&0xFF00) > 0);
    int16_t ovfw = (int16_t)(int8_t)a + (int16_t)(int8_t)b + (int16_t)car;
    setflag(cpu, FLAG_OFW, ovfw < -128 || ovfw > 127);
    return res;
}

int8_t do_sub_carry(
    struct ricoh_state *cpu,
    uint8_t a, uint8_t b,
    bool overflow,
    bool carry
)
{
    uint8_t car = (!getflag(cpu, FLAG_CAR)) && carry;
    int8_t res = (int8_t)a - (int8_t)b - (int8_t)car;
    uint16_t carrybits = (uint16_t)a - (uint16_t)b - (uint16_t)car;
    setflag(cpu, FLAG_NEG, res < 0);
    setflag(cpu, FLAG_ZER, res == 0);
    setflag(cpu, FLAG_CAR, (carrybits&0xFF00) == 0);
    if (overflow) {
        int16_t ovfw = (int16_t)(int8_t)a - (int16_t)(int8_t)b - (int16_t)car;
        setflag(cpu, FLAG_OFW, ovfw < -128 || ovfw > 127);
    }
    return res;
}

static void do_reljump(struct ricoh_state *cpu, struct instr_decoded instr, bool is)
{
    if (is) {
        cpu->pc = pagecrossbranch(cpu, cpu->pc, instr.operand[0]);
    }
}

static void do_cmp(struct ricoh_state *cpu, int8_t reg, int8_t operand)
{
    do_sub_carry(cpu, reg, operand, false, false);
}

void ricoh_do_interrupt(
    struct ricoh_state *cpu,
    struct ricoh_mem_interface *mem,
    uint16_t newpc
)
{
    push16(cpu, mem, cpu->pc);
    push8(cpu, mem, cpu->flags | (1 << FLAG_BRK) | (1 << FLAG_BI5));
    cpu->pc = newpc;
    cpu->cycles += 7;
}

void ricoh_run_instr(
    struct ricoh_state *cpu,
    struct instr_decoded instr,
    struct ricoh_mem_interface *mem
)
{
    cpu->pc += instr.size;
    cpu->cycles += ricoh_cycle_tbl[instr.addr_mode+instr.id*13];

    switch (instr.id)
    {
        case ADC:
            setreg(cpu, REG_A, do_add_carry(cpu, cpu->a, do_read(cpu, instr, mem)));
            break;
        case AND:
            setreg(cpu, REG_A, cpu->a & do_read(cpu, instr, mem));
            break;
        case ASL:
            {
                uint8_t byte = do_read(cpu, instr, mem);
                do_write(cpu, instr, mem, updateflags(cpu, byte<<1));
                setflag(cpu, FLAG_CAR, (byte&0x80) > 0);
            }
            break;
        case BCC:
            do_reljump(cpu, instr, getflag(cpu, FLAG_CAR) == false);
            break;
        case BCS:
            do_reljump(cpu, instr, getflag(cpu, FLAG_CAR) == true);
            break;
        case BEQ:
            do_reljump(cpu, instr, getflag(cpu, FLAG_ZER) == true);
            break;
        case BIT:
            {
                uint8_t byte = do_read(cpu, instr, mem);
                setflag(cpu, FLAG_NEG, byte>>7&1);
                setflag(cpu, FLAG_OFW, byte>>6&1);
                setflag(cpu, FLAG_ZER, (byte & cpu->a) == 0);
            }
            break;
        case BMI:
            do_reljump(cpu, instr, getflag(cpu, FLAG_NEG) == true);
            break;
        case BNE:
            do_reljump(cpu, instr, getflag(cpu, FLAG_ZER) == false);
            break;
        case BPL:
            do_reljump(cpu, instr, getflag(cpu, FLAG_NEG) == false);
            break;
        case BRK:
            printf("REGISTER DUMP: A=%02X X=%02X Y=%02X F=%02X P=%04X\n", (unsigned int)cpu->a, (unsigned int)cpu->x, (unsigned int)cpu->y, (unsigned int)cpu->flags, (unsigned int)cpu->pc);
            printf("RESULTS: %2X %2X\n", read_8(cpu, mem, 2), read_8(cpu, mem, 3));
            fflush(stdout);
            cpu->crash = 1;
            break;
        case BVC:
            do_reljump(cpu, instr, getflag(cpu, FLAG_OFW) == false);
            break;
        case BVS:
            do_reljump(cpu, instr, getflag(cpu, FLAG_OFW) == true);
            break;
        case CLC:
            setflag(cpu, FLAG_CAR, false);
            break;
        case CLD:
            setflag(cpu, FLAG_DEC, false);
            break;
        case CLI:
            setflag(cpu, FLAG_INT, false);
            break;
        case CLV:
            setflag(cpu, FLAG_OFW, false);
            break;
        case CMP:
            do_cmp(cpu, cpu->a, do_read(cpu, instr, mem));
            break;
        case CPX:
            do_cmp(cpu, cpu->x, do_read(cpu, instr, mem));
            break;
        case CPY:
            do_cmp(cpu, cpu->y, do_read(cpu, instr, mem));
            break;
        case DEC:
            do_write(cpu, instr, mem, updateflags(cpu,do_read(cpu, instr, mem)-1));
            break;
        case DEX:
            setreg(cpu, REG_X, cpu->x-1);
            break;
        case DEY:
            setreg(cpu, REG_Y, cpu->y-1);
            break;
        case EOR:
            setreg(cpu, REG_A, cpu->a ^ do_read(cpu, instr, mem));
            break;
        case INC:
            do_write(cpu, instr, mem, updateflags(cpu, do_read(cpu, instr, mem)+1));
            break;
        case INX:
            setreg(cpu, REG_X, cpu->x+1);
            break;
        case INY:
            setreg(cpu, REG_Y, cpu->y+1);
            break;
        case JMP:
            cpu->pc = do_readjmp(cpu, instr, mem);
            break;
        case JSR:
            push16(cpu, mem, cpu->pc-1);
            cpu->pc = do_readjmp(cpu, instr, mem);
            break;
        case LDA:
            setreg(cpu, REG_A, do_read(cpu, instr, mem));
            break;
        case LDX:
            setreg(cpu, REG_X, do_read(cpu, instr, mem));
            break;
        case LDY:
            setreg(cpu, REG_Y, do_read(cpu, instr, mem));
            break;
        case LSR:
            {
                uint8_t byte = do_read(cpu, instr, mem);
                do_write(cpu, instr, mem, updateflags(cpu, byte>>1));
                setflag(cpu, FLAG_CAR, (byte&1) > 0);
            }
            break;
        case NOP:
            break;
        case ORA:
            setreg(cpu, REG_A, cpu->a | do_read(cpu, instr, mem));
            break;
        case PHA:
            push8(cpu, mem, cpu->a);
            break;
        case PHP:
            push8(cpu, mem, cpu->flags | (1 << FLAG_BRK) | (1 << FLAG_BI5));
            break;
        case PLA:
            setreg(cpu, REG_A, pull8(cpu, mem));
            break;
        case PLP:
            cpu->flags = (cpu->flags & ((1 << FLAG_BRK) | (1 << FLAG_BI5))) | (pull8(cpu, mem) & (((1 << FLAG_BRK) | (1 << FLAG_BI5)) ^ 0xFF));
            break;
        case ROL:
            {
                uint8_t val = do_read(cpu, instr, mem);
                uint8_t car = getflag(cpu, FLAG_CAR);
                do_write(cpu, instr, mem, updateflags(cpu, (val<<1)|car));
                setflag(cpu, FLAG_CAR, (val&0x80) > 0);
            }
            break;
        case ROR:
            {
                uint8_t val = do_read(cpu, instr, mem);
                uint8_t car = getflag(cpu, FLAG_CAR);
                do_write(cpu, instr, mem, updateflags(cpu, (val>>1)|(car<<7)));
                setflag(cpu, FLAG_CAR, (val&0x1) > 0);
            }
            break;
        case RTI:
            cpu->flags = (cpu->flags & ((1 << FLAG_BRK) | (1 << FLAG_BI5))) | (pull8(cpu, mem) & (((1 << FLAG_BRK) | (1 << FLAG_BI5)) ^ 0xFF));

            cpu->pc = pull16(cpu, mem);
            break;
        case RTS:
            cpu->pc = pull16(cpu, mem)+1;
            break;
        case SBC:
            setreg(cpu, REG_A, do_sub_carry(cpu, cpu->a, do_read(cpu, instr, mem), true, true));
            break;
        case SEC:
            setflag(cpu, FLAG_CAR, 1);
            break;
        case SED:
            setflag(cpu, FLAG_DEC, 1);
            break;
        case SEI:
            setflag(cpu, FLAG_INT, 1);
            break;
        case STA:
            do_write(cpu, instr, mem, cpu->a);
            break;
        case STX:
            do_write(cpu, instr, mem, cpu->x);
            break;
        case STY:
            do_write(cpu, instr, mem, cpu->y);
            break;
        case TAX:
            setreg(cpu, REG_X, cpu->a);
            break;
        case TAY:
            setreg(cpu, REG_Y, cpu->a);
            break;
        case TSX:
            setreg(cpu, REG_X, cpu->sp);
            break;
        case TXA:
            setreg(cpu, REG_A, cpu->x);
            break;
        case TXS:
            cpu->sp = cpu->x;
            break;
        case TYA:
            setreg(cpu, REG_A, cpu->y);
            cpu->a = cpu->y;
            break;
        case _ICOUNT:
            assert(false && "not an instruction");
            break;
    }
}