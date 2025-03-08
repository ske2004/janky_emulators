#include "neske.h"

#define APU_FLAG_DMC    (1 << 4)
#define APU_FLAG_NOISE  (1 << 3)
#define APU_FLAG_TRI    (1 << 2)
#define APU_FLAG_PULSE2 (1 << 1)
#define APU_FLAG_PULSE1 (1 << 0)

const uint8_t duty_cycles[4] = { 0x40, 0x60, 0x78, 0x9F };

uint8_t duty_get_cycle(uint8_t duty, uint8_t cycle)
{
    return duty_cycles[duty] & (1 << cycle);
}


void apu_reg_write(struct apu *apu, enum apu_reg reg, uint8_t value)
{
    switch (reg)
    {
    case APU_PULSE1_DDLC_NNNN:
        apu->pulse1.duty                      = (value & 0xC0) >> 6;
        apu->pulse1.envl_loop                 = (value & 0x20) >> 5;
        apu->pulse1.envl_use_volume_or_period = (value & 0x10) >> 4;
        apu->pulse1.envl_volume_or_period     = (value & 0x0F);
        break;
    case APU_PULSE1_EPPP_NSSS:
        apu->pulse1.sweep_enable              = (value & 0x80) >> 7;
        apu->pulse1.sweep_period              = (value & 0x70) >> 4;
        apu->pulse1.sweep_negate              = (value & 0x08) >> 3;
        apu->pulse1.sweep_shift               = (value & 0x07);
        break;
    case APU_PULSE1_LLLL_LLLL:
        apu->pulse1.timer_init                = value;
        break;
    case APU_PULSE1_LLLL_LHHH:
        apu->pulse1.timer_init               |= (value&0x07) << 8;
        apu->pulse1.length                    = (value&0xF8);
        break;
    case APU_STATUS_IFXD_NT21:
        apu->status = value;
        break;
    }
}

uint8_t apu_reg_read(struct apu *apu, enum apu_reg reg)
{
    switch (reg)
    {
    case APU_STATUS_IFXD_NT21:
        return apu->pulse1.length > 0;
    default:
        break;
    }

    return 0;
}



void apu_cycle(struct apu *apu, struct apu_writer *out)
{
    uint32_t cycle_treshold = apu->last_frame_counter_cycles + 

    apu->cycles++;

    apu->pulse1.timer--;
    if (apu->pulse1.timer > apu->pulse1.timer_init)
    {
        apu->pulse1.duty_cycle++;
        apu->pulse1.timer = apu->pulse1.timer_init;
    }   
    
    apu->pulse1.duty_cycle %= 8;

    uint8_t volume = apu->pulse1.envl_volume_or_period*17;

    uint8_t sample  = duty_get_cycle(apu->pulse1.duty, apu->pulse1.duty_cycle)&volume;

    out->write(out->userdata, &sample, 1);
}
