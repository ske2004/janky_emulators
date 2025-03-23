#include "neske.h"
#include <stdio.h>

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

static void pulse_reg_write(struct apu_pulse_chan *pulse, uint8_t reg, uint8_t value)
{
    switch (reg)
    {
    case 0:
        pulse->duty                  = (value >> 6)&0x3;
        pulse->envl_halt             = (value >> 5)&1;
        pulse->envl_constant         = (value >> 4)&1;
        pulse->envl_volume_or_period = (value & 0x0F);
        if (pulse->envl_constant) {
            pulse->volume = pulse->envl_volume_or_period;
        } else {
            pulse->period = pulse->envl_volume_or_period;
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
        pulse->timer_init             = value;
        pulse->timer                  = pulse->timer_init;
        // printf("P1T %d\n", pulse->timer_init);
        break;
    case 3:
        if (pulse->envl_constant) {
            pulse->volume = pulse->envl_volume_or_period;
        } else {
            pulse->period = pulse->envl_volume_or_period;
            pulse->volume = 15;
        }
        pulse->timer_init            &= 0xFF;
        pulse->timer_init            |= (value&0x7) << 8;
        pulse->timer                 = pulse->timer_init;
        pulse->length                = length_lut[value>>3];
        pulse->duty_cycle            = 0; // reset duty cycle
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
        // printf("T1A: %d %d\n", apu->tri.flag_control, apu->tri.counter_init);
        break;
    case APU_TRIANG_LLLL_LLLL:
        apu->tri.timer_init = value;
        apu->tri.timer       = apu->tri.timer_init;
        // printf("T1B: %d\n", value);
        break;
    case APU_TRIANG_LLLL_LHHH:
        apu->tri.flag_reload = 1;
        apu->tri.length = length_lut[value>>3];
        apu->tri.timer_init &= 0xFF;
        apu->tri.timer_init |= (value&0x7)<<8;
        apu->tri.timer       = apu->tri.timer_init;
        // printf("T1C: %d %d\n", apu->tri.length, apu->tri.timer_init);
        break;

    case APU_STATUS_IFXD_NT21:
        if ((value & 1) == 0)
        {
            apu->pulse1.length = 0;
            apu->pulse1.envl_halt = 1;
        }
        if ((value & 2) == 0)
        {
            apu->pulse2.length = 0;
            apu->pulse2.envl_halt = 1;
        }
        if ((value & 4) == 0)
        {
            apu->tri.length = 0;
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

static uint8_t read_sample(struct apu *apu)
{   
    uint8_t pulse1 = 0;
    uint8_t pulse2 = 0;
    
    if (!apu->pulse1.sweep_lock)
    {
        pulse1 = duty_get_cycle(apu->pulse1.duty, apu->pulse1.duty_cycle) * apu->pulse1.volume;
    }
    
    if (!apu->pulse2.sweep_lock)
    {
        pulse2 = duty_get_cycle(apu->pulse2.duty, apu->pulse2.duty_cycle) * apu->pulse2.volume;
    }

    uint8_t tri = 0;

    tri = apu->tri.sequence-16;
    if (apu->tri.sequence < 16) tri = 15-apu->tri.sequence;

    float pulse = 95.88/((8128.0/((float)(pulse1 + pulse2)))+100.0);
    // i don't implement DMC (TODO)
    float tri_noise_dmc = 0;//159.79/(1.0/(tri/8227.0)+100.0);

    int sample = (pulse + tri_noise_dmc) * 255;

    if (sample < 0)
    {
        sample = 0;
    }

    if (sample > 255)
    {
        sample = 255;
    }

    return sample;
}

static void pulse_envelope_cycle(struct apu_pulse_chan *pulse)
{
    if (!pulse->envl_constant)
    {
        if (pulse->period == 0)
        {
            pulse->period = pulse->envl_volume_or_period;
            if (pulse->volume > 0)
            {
                pulse->volume -= 1;
            }
        }
        else 
        {
            pulse->period -= 1;
        }

        if (pulse->envl_halt && pulse->volume == 0)
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

    pulse->sweep_lock = false;

    if (pulse->sweep_enable && pulse->sweep_clock == 0)
    {
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

    if (tri->length > 0)
    {
        tri->length--;
    }
}

void apu_init(struct apu *apu)
{
    apu->pulse1.sweep_onecomp = 1;
}

static void pulse_clock(struct apu_pulse_chan *pulse)
{
    if (pulse->length == 0 || pulse->sweep_lock)
    {
        return;
    }

    pulse->timer--;
    if (pulse->timer > pulse->timer_init)
    {
        pulse->duty_cycle++;
        pulse->timer = pulse->timer_init;
    }   
    
    pulse->duty_cycle %= 8;
}

static void tri_clock(struct apu_tri_chan *tri)
{
    tri->timer--;
    if (tri->timer > tri->timer_init)
    {
        tri->sequence++;
        tri->timer = tri->timer_init;
    }
 
    if (tri->length == 0 || tri->counter == 0)
    {
        return;
    }
   
    tri->sequence %= 32;
}

static void envelope_cycle(struct apu *apu)
{
    pulse_envelope_cycle(&apu->pulse1);
    pulse_envelope_cycle(&apu->pulse2);
}

static void length_sweep_cycle(struct apu *apu)
{
    pulse_length_sweep_cycle(&apu->pulse1);
    pulse_length_sweep_cycle(&apu->pulse2);
    tri_length_cycle(&apu->tri);
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
    apu->cycles++;

    if ((apu->cycles & 1) == 1)
    {
        pulse_clock(&apu->pulse1);
        pulse_clock(&apu->pulse2);
    }

    tri_clock(&apu->tri);

    // dats cycles per frame
    uint32_t cpf = 1789773 / 240;
    uint32_t cpf_treshold = apu->last_cpf + cpf;
    
    // this is cycles per sample
    // 44.1 khz for the target sample
    uint32_t cps = 1789773 / 44100;
    uint32_t cps_treshold = apu->last_cps + cps;

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
