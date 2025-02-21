#ifndef INCLUDED_NESKE_H
#define INCLUDED_NESKE_H

#include <stdint.h>
#include <stdbool.h>

// RICOH.H

#define ADDR_MODE_COUNT 13

enum instr
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
};

enum addr_mode
{
    AM_ACC, AM_ABS, AM_ABX, AM_ABY, AM_IMM, AM_IMP, AM_IND, AM_XND, AM_INY, AM_REL, AM_ZPG, AM_ZPX, AM_ZPY
};

enum flags
{
    FLAG_CAR, FLAG_ZER, FLAG_INT, FLAG_DEC, FLAG_BRK, FLAG_BI5, FLAG_OFW, FLAG_NEG
};

struct instr_decoded
{
    enum instr id;
    enum addr_mode addr_mode;
    uint8_t operand[2];
    size_t size;
};

struct ricoh_decoder
{
    uint8_t itbl[256];
    uint8_t atbl[256];
};

struct ricoh_state
{
    uint16_t pc;
    uint8_t a, x, y, sp, flags;
    uint32_t cycles;
};

struct ricoh_mem_interface
{
    void *instance;
    uint8_t (*get)(void *instance, uint16_t addr);
    void (*set)(void *instance, uint16_t addr, uint8_t byte);
};

struct ricoh_decoder make_ricoh_decoder();
const char *ricoh_instr_name(enum instr instr);
struct instr_decoded ricoh_decode_instr(struct ricoh_decoder *decoder, struct ricoh_mem_interface *mem, uint16_t addr);
void ricoh_format_decoded_instr(char *dest, struct instr_decoded decoded);
void ricoh_do_interrupt(
    struct ricoh_state *cpu,
    struct ricoh_mem_interface *mem,
    uint16_t newpc
);
void ricoh_run_instr(
    struct ricoh_state *cpu,
    struct instr_decoded instr,
    struct ricoh_mem_interface *mem
);

// PPU.H

enum ppu_ir
{
    PPUIR_CTRL,
    PPUIR_MASK,
    PPUIR_STATUS,
    PPUIR_OAMADDR,
    PPUIR_OAMDATA,
    PPUIR_SCROLLXADDRHI,
    PPUIR_SCROLLYADDRLO,
    PPUIR_DATA,
    PPUIR_OAMDMA,
    PPUIR_COUNT
};

enum ppu_io
{
    PPUIO_CTRL,
    PPUIO_MASK,
    PPUIO_STATUS,
    PPUIO_OAMADDR,
    PPUIO_OAMDATA,
    PPUIO_SCROLL,
    PPUIO_ADDR,
    PPUIO_DATA,
    PPUIO_OAMDMA,
    PPUIO_COUNT
};


enum ppu_mir
{
    PPUMIR_VER,
    PPUMIR_HOR,
};


struct ppu
{
    uint8_t oam[256];
    uint8_t vram[1<<14];
    uint8_t extlatch;
    uint8_t regs[PPUIR_COUNT];
    uint16_t beam;
    int16_t scanline;
    uint32_t ticks;
    uint8_t screen[256*240];
    enum ppu_mir mirroring_mode;
};

struct ppu ppu_mk(enum ppu_mir mirroring_mode);
void ppu_write_chr(struct ppu *ppu, uint8_t *chr, uint32_t chr_size);
uint16_t ppu_get_addr(struct ppu *ppu);
void ppu_set_addr(struct ppu *ppu, uint16_t addr);
void ppu_write(struct ppu *ppu, enum ppu_io io, uint8_t data);
void ppu_vblank(struct ppu *ppu);
uint8_t ppu_vram_read(struct ppu *ppu, uint16_t addr);
void ppu_vram_write(struct ppu *ppu, uint16_t addr, uint8_t val);
uint8_t ppu_read(struct ppu *ppu, enum ppu_io io);
bool ppu_nmi_enabled(struct ppu *ppu);
void ppu_write_oam(struct ppu *ppu, uint8_t *oamsrc);
bool ppu_cycle(struct ppu *ppu, struct ricoh_mem_interface *mem);

// IMAP.H

struct print_instr
{
    uint16_t at;
    struct print_instr *next;
    struct print_instr *prev;
    char value[512];
};

struct imap
{
    struct print_instr *addr[1<<16];
};

struct imap *imap_mk();
void imap_populate(struct imap *imap, struct ricoh_decoder *decoder, struct ricoh_mem_interface *mem, uint16_t entry);
void imap_list_range(struct imap *imap, uint16_t entry, struct print_instr **dest, int from, int to);

// NROM.H

enum vector
{
    VEC_NMI,
    VEC_RESET,
    VEC_IRQ
};

enum controller_btn
{
    BTN_A,
    BTN_B,
    BTN_SELECT,
    BTN_START,
    BTN_UP,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
};

struct controller_state
{
    uint8_t btns[8];
};

struct nrom
{
    struct ricoh_decoder decoder;
    struct ricoh_state cpu;
    struct ppu ppu;
    
    struct controller_state controller;
    uint8_t controller_strobe;

    uint8_t prgsize;
    uint8_t chrsize;
    uint8_t memory[1<<16];
};

struct nrom_frame_result
{
    uint8_t screen[240*256];
};

uint16_t nrom_get_vector(struct nrom *nrom, enum vector vec);
void nrom_update_controller(struct nrom *nrom, struct controller_state cs);
uint8_t nrom_load(uint8_t *ines, struct nrom *out);
struct ricoh_mem_interface nrom_get_memory_interface(struct nrom *nrom);
struct nrom_frame_result nrom_frame(struct nrom *nrom);

#endif