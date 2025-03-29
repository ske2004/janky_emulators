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
    uint64_t cycles;

    uint8_t crash;
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

struct ppu_object
{
    uint8_t y;
    uint8_t tile;
    uint8_t attr;
    uint8_t x;
};

struct ppu
{
    // Internal memory
    struct ppu_object oam[64];
    uint8_t vram[1<<14];
    uint8_t regs[PPUIR_COUNT];

    // Internal Registers   
    uint16_t t;
    uint8_t x;
    uint8_t w;
    uint32_t v;

    // Rendering & Timing
    uint8_t screen[256*240];
    enum ppu_mir mirroring_mode;
    uint16_t beam;
    int16_t scanline;
    uint64_t cycles;
    struct ppu_object preload_objects[8];
    uint8_t preload_objects_sprite_0;
    uint8_t preload_objects_count;
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

// APU.H

enum apu_reg
{
    APU_PULSE1_DDLC_NNNN,
    APU_PULSE1_EPPP_NSSS,
    APU_PULSE1_LLLL_LLLL,
    APU_PULSE1_LLLL_LHHH,

    APU_PULSE2_DDLC_NNNN,
    APU_PULSE2_EPPP_NSSS,
    APU_PULSE2_LLLL_LLLL,
    APU_PULSE2_LLLL_LHHH,
    
    APU_TRIANG_CRRR_RRRR,
    APU_TRIANG_LLLL_LLLL,
    APU_TRIANG_LLLL_LHHH,

    APU_NOISER_XXLC_VVVV,
    APU_NOISER_MXXX_PPPP,
    APU_NOISER_LLLL_LXXX,

    APU_STATUS_IFXD_NT21,
    APU_STATUS_MIXX_XXXX,
};

struct apu_tri_chan
{
    uint8_t flag_control;
    uint8_t flag_reload;

    uint16_t timer_init;
    uint8_t counter_init;
    uint8_t length;

    // internal
    uint16_t timer;
    uint8_t counter;
    uint8_t sequence;
    uint8_t enabled;
};

struct apu_noise_chan
{
    uint16_t lfsr;

    uint8_t envl_halt;
    uint8_t envl_constant;
    uint8_t envl_volume_or_period;
    uint8_t length;
    uint8_t mode;

    uint16_t timer;
    uint16_t timer_init;

    // internal
    uint8_t flag_start;
    uint8_t period;
    uint8_t decay;
    uint8_t enabled;
};

struct apu_pulse_chan
{
    uint8_t sweep_enable;
    uint8_t sweep_period;
    uint8_t sweep_negate;
    uint8_t sweep_shift;

    uint8_t envl_halt;
    uint8_t envl_constant;
    uint8_t envl_volume_or_period;

    uint8_t duty;
    uint8_t length;
    uint16_t timer_init;

    // internal
    uint8_t sweep_reload;
    uint8_t sweep_lock;
    uint8_t sweep_onecomp;
    uint8_t sweep_clock;
    uint16_t timer;
    uint8_t duty_cycle;
    uint8_t decay;
    uint8_t period;
    uint8_t flag_start;
    uint8_t enabled;
};

struct apu_writer
{
    void *userdata;
    void (*write)(void *userdata, uint8_t *samples, uint32_t count);
};

#define APU_SAMPLE_RING_LEN (16384)

struct apu_high_pass
{
    float last_in;
    float last_out;
};

struct apu
{
    uint8_t flag_enable_interrupt;
    uint8_t flag_counter_mode_2;
    uint8_t flag_frame_interrupt;

    uint32_t frame_counter;
    uint8_t status;
    uint64_t last_cpf; // last cycle of frame clock
    uint64_t last_cps; // last cycle of sample output
    uint64_t cycles;

    int16_t sample_ring[APU_SAMPLE_RING_LEN];
    uint32_t sample_ring_write_at;
    uint32_t sample_ring_read_at;
    uint32_t samples_written;
    uint32_t samples_read;

    struct apu_pulse_chan pulse1;
    struct apu_pulse_chan pulse2;
    struct apu_tri_chan tri;
    struct apu_noise_chan noise;

    struct apu_high_pass high_pass;
};

void apu_init(struct apu *apu);
void apu_reg_write(struct apu *apu, enum apu_reg reg, uint8_t value);
uint8_t apu_reg_read(struct apu *apu, enum apu_reg reg);
void apu_flush(struct apu *apu, void *dest, int count);
void apu_cycle(struct apu *apu);
void apu_ring_read(struct apu *apu, uint16_t *dest, uint32_t count);
void apu_catchup_cycles(struct apu *apu, uint64_t cycles);
void apu_catchup_samples(struct apu *apu, uint32_t samples_added);

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

// MUX.H

struct mux_api {
    void *mux;
    void (*lock)(void *mux);
    void (*unlock)(void *mux);
};

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
    struct apu apu;
    struct mux_api apu_mux;

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
void nrom_reset(struct nrom *nrom);

#endif