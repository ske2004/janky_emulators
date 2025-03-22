#include "neske.h"

#define APU_FLAG_DMC    (1 << 4)
#define APU_FLAG_NOISE  (1 << 3)
#define APU_FLAG_TRI    (1 << 2)
#define APU_FLAG_PULSE2 (1 << 1)
#define APU_FLAG_PULSE1 (1 << 0)

static const uint8_t duty_cycles[4] = { 0x40, 0x60, 0x78, 0x9F };

static const uint8_t length_lut[] = {
    10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static uint8_t duty_get_cycle(uint8_t duty, uint8_t cycle)
{
    return (duty_cycles[duty] & (1 << cycle)) > 0;
}

void apu_reg_write(struct apu *apu, enum apu_reg reg, uint8_t value)
{
    switch (reg)
    {
    case APU_PULSE1_DDLC_NNNN:
        apu->pulse1.duty                  = (value >> 6)&0x3;
        apu->pulse1.envl_halt             = (value >> 5)&1;
        apu->pulse1.envl_constant         = (value >> 4)&1;
        apu->pulse1.envl_volume_or_period = (value & 0x0F);
        if (apu->pulse1.envl_constant) {
            apu->pulse1.volume = apu->pulse1.envl_volume_or_period;
        } else {
            apu->pulse1.period = apu->pulse1.envl_volume_or_period;
            apu->pulse1.volume = 15;
        }
        // printf("P1A: %d %d %d %d\n", apu->pulse1.duty, apu->pulse1.envl_halt, apu->pulse1.envl_constant, apu->pulse1.envl_volume_or_period);
        break;
    case APU_PULSE1_EPPP_NSSS:
        apu->pulse1.sweep_enable              = (value >> 7)&1;
        apu->pulse1.sweep_period              = (value >> 4)&0x7;
        apu->pulse1.sweep_clock               = apu->pulse1.sweep_period;
        apu->pulse1.sweep_negate              = (value >> 3)&1;
        apu->pulse1.sweep_shift               = (value >> 0)&0x7;
        // printf("P1S: %d %d %d %d\n", apu->pulse1.sweep_enable, apu->pulse1.sweep_period, apu->pulse1.sweep_negate, apu->pulse1.sweep_shift);
        break;
    case APU_PULSE1_LLLL_LLLL:
        apu->pulse1.timer_init                = value;
        apu->pulse1.timer = apu->pulse1.timer_init;
        break;
    case APU_PULSE1_LLLL_LHHH:
        // TODO: Reset envelope
        apu->pulse1.timer_init               |= (value&0x7) << 8;
        apu->pulse1.timer = apu->pulse1.timer_init;
        apu->pulse1.length                    = length_lut[(value&0xF8)>>3];
        apu->pulse1.duty_cycle = 0; // reset duty cycle
        // printf("P1D: %d %d\n", apu->pulse1.timer_init, apu->pulse1.length);
        break;
    case APU_STATUS_IFXD_NT21:
        if (value & 1) {
            apu->pulse1.length = 0;
        }
        apu->status = value;
        break;
    case APU_STATUS_MIXX_XXXX:
        // TODO: Interrupt
        apu->flag_counter_mode_2 = value >> 7;
        apu->flag_enable_interrupt = (value >> 6) & 1;
        break;
    }
}


uint8_t apu_reg_read(struct apu *apu, enum apu_reg reg)
{
    switch (reg)
    {
    case APU_STATUS_IFXD_NT21:
        {
            uint8_t flags = 0;
            // pulse 1 enabled
            flags |= apu->pulse1.length > 0;
            // interrupt inhibit flag
            flags |= apu->flag_frame_interrupt << 6;
            apu->flag_frame_interrupt = 0;
            return flags;
        }
    default:
        break;
    }

    return 0;
}

static uint8_t read_volume(struct apu *apu)
{
    if (apu->pulse1.sweep_enable) {
        return 0xF;
    }

    return apu->pulse1.envl_volume_or_period*15;
}

static uint8_t read_sample(struct apu *apu)
{   
    if (apu->pulse1.length > 0) {
        uint8_t sample = duty_get_cycle(apu->pulse1.duty, apu->pulse1.duty_cycle) * apu->pulse1.volume;
        return sample;
    }

    return 0;
}

static void pulse_envelope_cycle(struct apu_pulse_chan *pulse)
{
    if (!pulse->envl_constant)
    {
        // if envelope
        pulse->period -= 1;
        if (pulse->period == 0)
        {
            pulse->period = pulse->envl_volume_or_period;
            pulse->volume -= 1;
        }

        if (pulse->volume == 0)
        {
            pulse->volume = 15;
        }
    }
}

static void pulse_length_sweep_cycle(struct apu_pulse_chan *pulse)
{
    if (!pulse->envl_halt && pulse->length > 0) 
    {
        pulse->length--;
    }

    if (pulse->sweep_enable)
    {
        if (pulse->sweep_clock == 0) 
        {
            pulse->sweep_clock = pulse->sweep_period;

            int change = pulse->timer >> pulse->sweep_shift;
            if (pulse->sweep_negate)
            {
                change = -change;
            }

            int sum = pulse->timer_init + change;

            if (sum < 8)
            {
                sum = 8;
                // HACK?
                pulse->length = 0;
            }
            if (sum > 0x7FF)
            {
                sum = 0x7FF;
                // HACK AGAIN??
                pulse->length = 0;
            }

            pulse->timer_init = sum;
        }
        else
        {
            pulse->sweep_clock -= 1;
        }
    }
}

static void envelope_cycle(struct apu *apu)
{
    pulse_envelope_cycle(&apu->pulse1);
}

static void length_sweep_cycle(struct apu *apu)
{
    pulse_length_sweep_cycle(&apu->pulse1);
}

static void frame_cycle(struct apu *apu)
{
    if (apu->flag_counter_mode_2 == 0) 
    {
        switch (apu->frame_counter%4) 
        {
        case 0: envelope_cycle(apu); length_sweep_cycle(apu); break;
        case 1: envelope_cycle(apu); break;
        case 2: envelope_cycle(apu); length_sweep_cycle(apu); break;
        case 3: 
            apu->flag_frame_interrupt = 1;
            envelope_cycle(apu);
            break;
        }
    } else {
        switch (apu->frame_counter%5) 
        {
        case 0: envelope_cycle(apu); length_sweep_cycle(apu); break;
        case 1: envelope_cycle(apu); break;
        case 2: envelope_cycle(apu); length_sweep_cycle(apu); break;
        case 3: break;
        case 4: envelope_cycle(apu); break;
        }
    }

    apu->frame_counter++;
}

void apu_ring_write(struct apu *apu, uint8_t value)
{
    apu->sample_ring[apu->sample_ring_write_at++] = value;
    apu->sample_ring_write_at %= APU_SAMPLE_RING_LEN;
}

void apu_ring_read(struct apu *apu, uint8_t *dest, uint32_t count)
{
    int at = apu->sample_ring_read_at;

    while (at < 0) at += APU_SAMPLE_RING_LEN;

    for (int i = 0; i < count; i++) {
        at %= APU_SAMPLE_RING_LEN;
        *dest++ = apu->sample_ring[at++];
    }

    apu->sample_ring_read_at = at;
}

void apu_cycle(struct apu *apu)
{
    apu->cycles++;

    // dats cycles per frame
    uint32_t cpf = (1789773 / 2) / 240;
    uint32_t cpf_treshold = apu->last_cpf + cpf;
    
    // this is cycles per sample
    // 44.1 khz for the target sample, it's not even lol
    uint32_t cps = (1789773 / 2) / 44100;
    uint32_t cps_treshold = apu->last_cps + cps;

    apu->pulse1.timer--;
    if (apu->pulse1.timer > apu->pulse1.timer_init)
    {
        apu->pulse1.duty_cycle++;
        apu->pulse1.timer = apu->pulse1.timer_init;
    }   
    
    apu->pulse1.duty_cycle %= 8;

    if (apu->cycles > cpf_treshold)
    {
        apu->last_cpf = cpf_treshold;
        frame_cycle(apu);
    }

    if (apu->cycles > cps_treshold)
    {
        apu->last_cps = cps_treshold;
        apu_ring_write(apu, read_sample(apu));
        apu->samples_written_this_frame++;
    }  
}
