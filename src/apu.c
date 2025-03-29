#include "neske.h"
#include <stdio.h>

#define APU_FLAG_DMC    (1 << 4)
#define APU_FLAG_NOISE  (1 << 3)
#define APU_FLAG_TRI    (1 << 2)
#define APU_FLAG_PULSE2 (1 << 1)
#define APU_FLAG_PULSE1 (1 << 0)

static const uint8_t duty_cycles[4] = { 0x40, 0x60, 0x78, 0x9F };

static const uint8_t length_lut[] =
{
    10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static const uint16_t period_lut[] =
{
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

static uint8_t duty_get_cycle(uint8_t duty, uint8_t cycle)
{
    return (duty_cycles[duty] & (1 << cycle)) > 0;
}

static void pulse_reg_write(struct apu_pulse_chan *pulse, uint8_t reg, uint8_t value)
{
    switch (reg)
    {
    case 0:
        pulse->duty                  = (value >> 6)&0x3;
        pulse->envl_halt             = (value >> 5)&1;
        pulse->envl_constant         = (value >> 4)&1;
        pulse->envl_volume_or_period = (value & 0x0F);
        if (!pulse->envl_constant)
        {
            pulse->period            = pulse->envl_volume_or_period; // restart envelope
        }
        // printf("P1A: %d %d %d %d\n", pulse->duty, pulse->envl_halt, pulse->envl_constant, pulse->envl_volume_or_period);
        break;
    case 1:
        pulse->sweep_enable          = (value >> 7)&1;
        pulse->sweep_period          = (value >> 4)&0x7;
        pulse->sweep_negate          = (value >> 3)&1;
        pulse->sweep_shift           = (value >> 0)&0x7;
        pulse->sweep_reload          = 1;
        // printf("P1S: %d %d %d %d\n", pulse->sweep_enable, pulse->sweep_period, pulse->sweep_negate, pulse->sweep_shift);
        break;
    case 2:
        pulse->timer_init            = value;
        // pulse->timer                  = pulse->timer_init;
        // printf("P1T %d\n", pulse->timer_init);
        break;
    case 3:
        pulse->flag_start            = 1; // restart envelope
        pulse->timer_init           &= 0xFF;
        pulse->timer_init           |= (value&0x7) << 8;
        // pulse->timer                 = pulse->timer_init;
        if (pulse->enabled)
        {
            pulse->length            = length_lut[value>>3];
        }
        pulse->duty_cycle            = 0; // restart sequencer
        // printf("P1D: %d %d\n", pulse->timer_init, pulse->length);
        break;
    }
}

void apu_reg_write(struct apu *apu, enum apu_reg reg, uint8_t value)
{
    switch (reg)
    {
    case APU_PULSE1_DDLC_NNNN: pulse_reg_write(&apu->pulse1, 0, value); break;
    case APU_PULSE1_EPPP_NSSS: pulse_reg_write(&apu->pulse1, 1, value); break;
    case APU_PULSE1_LLLL_LLLL: pulse_reg_write(&apu->pulse1, 2, value); break;
    case APU_PULSE1_LLLL_LHHH: pulse_reg_write(&apu->pulse1, 3, value); break;

    case APU_PULSE2_DDLC_NNNN: pulse_reg_write(&apu->pulse2, 0, value); break;
    case APU_PULSE2_EPPP_NSSS: pulse_reg_write(&apu->pulse2, 1, value); break;
    case APU_PULSE2_LLLL_LLLL: pulse_reg_write(&apu->pulse2, 2, value); break;
    case APU_PULSE2_LLLL_LHHH: pulse_reg_write(&apu->pulse2, 3, value); break;

    case APU_TRIANG_CRRR_RRRR:
        apu->tri.flag_control = value >> 7;
        apu->tri.counter_init = value & 0x7F;
        // apu->tri.counter      = apu->tri.counter_init;
        // printf("T1A: %d %d\n", apu->tri.flag_control, apu->tri.counter_init);
        break;
    case APU_TRIANG_LLLL_LLLL:
        apu->tri.timer_init = value;
        apu->tri.timer      = apu->tri.timer_init;
        // printf("T1B: %d\n", value);
        break;
    case APU_TRIANG_LLLL_LHHH:
        apu->tri.flag_reload = 1;
        if (apu->tri.enabled)
        {
            apu->tri.length = length_lut[value>>3];
        }
        apu->tri.timer_init &= 0xFF;
        apu->tri.timer_init |= (value&0x7)<<8;
        apu->tri.timer       = apu->tri.timer_init;
        // printf("T1C: %d %d\n", apu->tri.length, apu->tri.timer_init);
        break;

    case APU_NOISER_XXLC_VVVV:
        apu->noise.envl_halt             = (value>>5)&1;
        apu->noise.envl_constant         = (value>>4)&1;
        apu->noise.envl_volume_or_period = (value)&0xF;
        if (!apu->noise.envl_constant)
        {
            apu->noise.period            = apu->noise.envl_volume_or_period; // restart envelope
        }
        // printf("N1A: %d %d %d\n", apu->noise.envl_halt, apu->noise.envl_constant, apu->noise.envl_volume_or_period);
        break;

    case APU_NOISER_MXXX_PPPP:
        apu->noise.timer_init = period_lut[value&0xF];
        apu->noise.mode       = value>>7;
        // printf("N1B: %d %d\n", apu->noise.timer_init, apu->noise.mode);
        break;

    case APU_NOISER_LLLL_LXXX:
        if (apu->noise.enabled)
        {
            apu->noise.length     = length_lut[value>>3];
        }
        apu->noise.flag_start = 1;
        // printf("N1C: %d\n", apu->noise.length);
        break;

    case APU_STATUS_IFXD_NT21:
        apu->pulse1.enabled = (value & 1) > 0;
        if (!apu->pulse1.enabled)
        {
            apu->pulse1.length = 0;
        }
        apu->pulse2.enabled = (value & 2) > 0;
        if (!apu->pulse2.enabled)
        {
            apu->pulse2.length = 0;
        }
        apu->tri.enabled = (value & 4) > 0;
        if (!apu->tri.enabled)
        {
            apu->tri.length = 0;
        }
        apu->noise.enabled = (value & 8) > 0;
        if (!apu->noise.enabled)
        {
            apu->noise.length = 0;
        }
        // TODO: i dont use this
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
            flags |= (apu->pulse1.length > 0 && !apu->pulse1.sweep_lock);
            // pulse 2 enabled
            flags |= (apu->pulse2.length > 0 && !apu->pulse2.sweep_lock)<<1;
            // tri enabled (TODO: do i count the internal counter as well?)
            flags |= (apu->tri.length > 0)<<2;
            // noise enabled
            flags |= (apu->noise.length > 0)<<3;
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

static float do_high_pass_filter(struct apu_high_pass *high_pass, float sample)
{
    float rc = 1.0/(37.0*2*3.1415);
    float dt = 1.0/44100.0;
    float alpha = rc/(rc + dt);
    float value = alpha * high_pass->last_out + alpha * (sample - high_pass->last_in);
    high_pass->last_in = sample;
    high_pass->last_out = value;
    return value;
}

static int16_t read_sample(struct apu *apu)
{   
    uint8_t pulse1 = 0;

    if (apu->pulse1.length != 0 && !apu->pulse1.sweep_lock)
    {
        uint8_t volume = apu->pulse1.envl_constant ? apu->pulse1.envl_volume_or_period : apu->pulse1.decay;
        pulse1 = duty_get_cycle(apu->pulse1.duty, apu->pulse1.duty_cycle) * volume;
    }
    
    uint8_t pulse2 = 0;
    
    if (apu->pulse2.length != 0 && !apu->pulse2.sweep_lock)
    {
        uint8_t volume = apu->pulse2.envl_constant ? apu->pulse2.envl_volume_or_period : apu->pulse2.decay;
        pulse2 = duty_get_cycle(apu->pulse2.duty, apu->pulse2.duty_cycle) * volume;
    }

    uint8_t tri = 0;

    if (apu->tri.length != 0)
    {
        tri = apu->tri.sequence-16;
        if (apu->tri.sequence < 16) tri = 15-apu->tri.sequence;
    }

    uint8_t noise = 0;

    if (apu->noise.length != 0 && (apu->noise.lfsr&1) != 0)
    {
        uint8_t volume = apu->noise.envl_constant ? apu->noise.envl_volume_or_period : apu->noise.decay;
        noise = volume;
    }

    float pulse = 95.88/((8128.0/((float)(pulse1 + pulse2)))+100.0);
    // i don't implement DMC (TODO)
    float tri_noise_dmc = 159.79/(1.0/((tri/8227.0)+(noise/12241.0))+100.0);

    float value = do_high_pass_filter(&apu->high_pass, pulse + tri_noise_dmc);

    if (value > 1.0) value = 1.0;
    if (value < -1.0) value = -1.0;

    int sample = value * 32766;

    return sample;
}

static void pulse_envelope_cycle(struct apu_pulse_chan *pulse)
{
    if (pulse->flag_start)
    {
        pulse->period = pulse->envl_volume_or_period;
        pulse->decay = 15;
        pulse->flag_start = 0;
    }
    else
    {
        if (pulse->period == 0)
        {
            pulse->period = pulse->envl_volume_or_period;

            if (pulse->decay > 0)
            {
                pulse->decay -= 1;
            }
            else if (pulse->envl_halt)
            {
                pulse->decay = 15;
            }
        }
        else 
        {
            pulse->period -= 1;
        }
    }
}

static void pulse_length_sweep_cycle(struct apu_pulse_chan *pulse)
{
    if (!pulse->envl_halt && pulse->length > 0) 
    {
        pulse->length--;
    }
    
    if (!pulse->sweep_enable) 
    {
        pulse->sweep_lock = false;
    }
    else if (pulse->sweep_clock == 0)
    {
        pulse->sweep_lock = false;

        int change = pulse->timer_init >> pulse->sweep_shift;
        if (pulse->sweep_negate)
        {
            change = pulse->sweep_onecomp ? -change-1 : -change;
        }

        int sum = pulse->timer_init + change;

        if (sum < 8)
        {
            pulse->sweep_lock = true;
        }
        else if (sum > 0x7FF)
        {
            pulse->sweep_lock = true;
        }
        
        if (!pulse->sweep_lock)
        {
            pulse->timer_init = sum;
        }
    }

    if (pulse->sweep_reload || pulse->sweep_clock == 0)
    {
        pulse->sweep_clock = pulse->sweep_period;
        pulse->sweep_reload = 0;
    }
    else
    {
        pulse->sweep_clock -= 1;
    }
}

static void tri_length_cycle(struct apu_tri_chan *tri)
{
    if (tri->flag_reload) 
    {
        tri->counter = tri->counter_init;
    }
    else if (tri->counter != 0)
    {
        tri->counter--;
    }

    if (!tri->flag_control)
    {
        tri->flag_reload = 0;
    }
}

static void noise_envelope_cycle(struct apu_noise_chan *noise)
{
    if (noise->flag_start)
    {
        noise->period = noise->envl_volume_or_period;
        noise->decay = 15;
        noise->flag_start = 0;
    }
    else
    {
        if (noise->period == 0)
        {
            noise->period = noise->envl_volume_or_period;

            if (noise->decay > 0)
            {
                noise->decay -= 1;
            }
            else if (noise->envl_halt)
            {
                noise->decay = 15;
            }
        }
        else 
        {
            noise->period -= 1;
        }
    }
}

void apu_init(struct apu *apu)
{
    apu->pulse1.sweep_onecomp = 1;
    apu->noise.lfsr = 1;
}

static void pulse_clock(struct apu_pulse_chan *pulse)
{
    pulse->timer--;
    if (pulse->timer > pulse->timer_init)
    {
        pulse->timer = pulse->timer_init;
        
        if (pulse->length == 0 || pulse->sweep_lock)
        {
            return;
        }       
        pulse->duty_cycle++;
        pulse->duty_cycle %= 8;
    }   
}

static void tri_clock(struct apu_tri_chan *tri)
{
    tri->timer--;
    if (tri->timer > tri->timer_init)
    {
        tri->timer = tri->timer_init;
        if (tri->length == 0 || tri->counter == 0)
        {
            return;
        }
   
        tri->sequence++;
        tri->sequence %= 32;
    }
}

static void noise_clock(struct apu_noise_chan *noise)
{
    noise->timer--;
    if (noise->timer > noise->timer_init)
    {
        noise->timer = noise->timer_init;
        if (noise->length == 0)
        {
            return;
        }

        uint16_t bit = noise->mode == 1 ? 6 : 1;
        uint16_t xor = (noise->lfsr & 1) ^ ((noise->lfsr >> bit) & 1);
        noise->lfsr >>= 1;
        noise->lfsr |= xor << 14; 
    }
}

static void envelope_cycle(struct apu *apu)
{
    pulse_envelope_cycle(&apu->pulse1);
    pulse_envelope_cycle(&apu->pulse2);
    tri_length_cycle(&apu->tri);
    noise_envelope_cycle(&apu->noise);
}

static void length_sweep_cycle(struct apu *apu)
{
    pulse_length_sweep_cycle(&apu->pulse1);
    pulse_length_sweep_cycle(&apu->pulse2);

    if (!apu->noise.envl_halt && apu->noise.length > 0)
    {
        apu->noise.length--;
    }

    if (apu->tri.length > 0)
    {
        apu->tri.length--;
    }
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

void apu_ring_write(struct apu *apu, uint16_t value)
{
    apu->sample_ring[apu->sample_ring_write_at++] = value;
    apu->sample_ring_write_at %= APU_SAMPLE_RING_LEN;
}

void apu_ring_read(struct apu *apu, uint16_t *dest, uint32_t count)
{
    int at = apu->sample_ring_read_at;

    while (at < 0) at += APU_SAMPLE_RING_LEN;

    for (int i = 0; i < count; i++)
    {
        at %= APU_SAMPLE_RING_LEN;
        *dest++ = apu->sample_ring[at++];
    }

    apu->sample_ring_read_at = at;
}

void apu_cycle(struct apu *apu)
{
    // triangle clocks at CPU speed so others need to clock at half CPU speed  
    // noise cycles in LUT are in CPU cycles
    apu->cycles++;

    if ((apu->cycles & 1) == 1)
    {
        pulse_clock(&apu->pulse1);
        pulse_clock(&apu->pulse2);
    }

    noise_clock(&apu->noise);
    tri_clock(&apu->tri);

    // dats cycles per frame
    uint64_t cpf = 1789773 / 240;
    uint64_t cpf_treshold = apu->last_cpf + cpf;
    
    // this is cycles per sample
    // 44.1 khz for the target sample
    uint64_t cps = 1789773 / 44100;
    uint64_t cps_treshold = apu->last_cps + cps;

    if (apu->cycles > cpf_treshold)
    {
        apu->last_cpf = cpf_treshold;
        frame_cycle(apu);
    }

    if (apu->cycles > cps_treshold)
    {
        apu->last_cps = cps_treshold;
        apu_ring_write(apu, read_sample(apu));
        apu->samples_written++;
    }  
}

// TODO: Not used, because still creates delayed and choppy sound if too
//       much samples submitted, this solution sacrifices accuracy ;-;
void apu_catchup_cycles(struct apu *apu, uint64_t cycles)
{
    while (apu->cycles < cycles)
    {
        apu_cycle(apu);
    }
}

void apu_catchup_samples(struct apu *apu, uint32_t samples_added)
{
    while (apu->samples_written < samples_added)
    {
        apu_cycle(apu);
    }
    apu->samples_written = 0;
}

