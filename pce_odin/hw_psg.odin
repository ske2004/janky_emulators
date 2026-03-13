package main 

import "core:fmt"
import "core:math/rand"
import "core:math"

PSG_SAMPLE_RATE :: 44100

PSG_Balance :: bit_field u8 {
  right: uint | 4,
  left:  uint | 4,
}

PSG_LFO_Flags :: bit_field u8 {
  ctrl:    bool | 1,
  unused:  u8   | 6,
  trigger: bool | 1,
}

PSG_Chan_Flags :: bit_field u8 {
  volume: u8   | 5,
  unused: uint | 1,
  dda_on: bool | 1, 
  on:     bool | 1,
}

PSG_Chan_Noise :: bit_field u8 {
  freq:   uint | 5,
  unused: uint | 2,
  on:     bool | 1,
}

PSG_Sample :: bit_field u8 {
  val: u8 | 5
}

PSG_Chan :: struct {
  samples:    [32]PSG_Sample,
  freq:       uint,
  flags:      PSG_Chan_Flags,
  balance:    PSG_Balance,
  noise:      PSG_Chan_Noise,
  sample_idx: uint,
  write_idx:  uint,
  
  _noise_state: u8
}

PSG :: struct {
  chan_selected:  int,
  channels:       [6]PSG_Chan,
  global_balance: PSG_Balance,
  lfo_freq:       u8,
  lfo_flags:      PSG_LFO_Flags,
  rng:            rand.Xoshiro256_Random_State,
  sample_buf:     [dynamic]f32,
  
  cycles:         uint,
  dac_start:      uint
}

psg_resample :: proc(buf_in, buf_out: []f32) {
  if len(buf_in) == 0 {
    return
  }

  l_in := len(buf_in)/2
  l_out := len(buf_out)/2

  for i:=0; i<l_out; i+=1 {
    t := cast(f32)i/cast(f32)l_out*cast(f32)l_in
    j := clamp(cast(int)t, 0, l_in-1)

    buf_out[i*2] = buf_in[j*2]
    buf_out[i*2+1] = buf_in[j*2+1]
  }
}

@(private="file")
powf10 :: proc(f: f32) -> f32 {
  return math.exp_f32(f * math.LN10)
}

// Convert decibel to linear factor
vol :: proc(k: f32, n: f32, rate: f32) -> f32 {
  return powf10(((k-n+1)*-rate/2)/10.0)
}

psg_get_chan :: proc(psg: ^PSG) -> ^PSG_Chan {
  if psg.chan_selected > 5 {
    return nil
  }

  return &psg.channels[psg.chan_selected]
}

psg_cycle_chan :: proc(psg: ^PSG, chan: ^PSG_Chan) {
  if chan.noise.on {
    t := (chan.noise.freq~0x1F)
    if t != 0 && psg.cycles%t == 0 {
      if chan.sample_idx == 0 {
        chan._noise_state = cast(u8)(rand.uint_max(2, rand.xoshiro256_random_generator(&psg.rng)))*31
      }
      chan.sample_idx += 1
      chan.sample_idx %= 32
    }
  } else {
    t := chan.freq
    if t != 0 && psg.cycles%t == 0 {
      chan.sample_idx += 1
      chan.sample_idx %= 32
    }
  }
}

psg_chan_read_sample :: proc(bus: ^Bus, psg: ^PSG, chan: uint) -> PSG_Sample {
  chan := psg.channels[chan]
  if chan.noise.on {
    return transmute(PSG_Sample)(chan._noise_state)
  } else {
    return chan.samples[chan.sample_idx]
  }
}

psg_chan_mix :: proc(bus: ^Bus, psg: ^PSG, chan: uint) -> (l, r: f32) {
  s := psg_chan_read_sample(bus, psg, cast(uint)chan).val
  sample := cast(f32)s/16

  chan := psg.channels[chan]

  if !chan.flags.on {
    return
  }

  l = sample*
      vol(31, cast(f32)chan.flags.volume, 1.5)*
      vol(15, cast(f32)chan.balance.left, 3);
  r = sample*
      vol(31, cast(f32)chan.flags.volume, 1.5)*
      vol(15, cast(f32)chan.balance.right, 3);

  return l, r
}

psg_flush :: proc(psg: ^PSG, buf_out: []f32) {
  // fmt.printf("SampleReq: %v -> %v\n", len(psg.sample_buf), len(buf_out))
  psg_resample(psg.sample_buf[:], buf_out)
  clear(&psg.sample_buf)
}

psg_read_sample :: proc(bus: ^Bus, psg: ^PSG, chan: uint) -> (l: f32, r: f32) {
  if chan == 0 {
    for chan, i in psg.channels {
      lt, rt := psg_chan_mix(bus, psg, uint(i))
      l += lt
      r += rt
    }

    l /= cast(f32)len(psg.channels)
    r /= cast(f32)len(psg.channels)
  } else {
    l, r = psg_chan_mix(bus, psg, chan-1)
  }

  l *= vol(15, cast(f32)psg.global_balance.left, 3)
  r *= vol(15, cast(f32)psg.global_balance.right, 3)

  return 
}

psg_cyc := uint(0)

psg_cycle :: proc(bus: ^Bus, psg: ^PSG) {
  CPS :: 3580000 / PSG_SAMPLE_RATE

  psg_cyc += 3

  for psg_cyc >= 6 {
    psg_cyc -= 6

    for &chan in psg.channels {
      psg_cycle_chan(psg, &chan)
    }

    if psg.cycles >= psg.dac_start + CPS {
      psg.dac_start += CPS
      
      l, r := psg_read_sample(bus, psg, 0)

      append(&psg.sample_buf, l, r)
    }
    
    psg.cycles += 1
  }
}

psg_read :: proc(bus: ^Bus, psg: ^PSG, adr: PSG_Addrs) -> u8 {
  return 0x00
}

psg_write :: proc(bus: ^Bus, psg: ^PSG, adr: PSG_Addrs, val: u8) {
  chan := psg_get_chan(psg)
 
  switch adr {
  case .Chan_Select: psg.chan_selected = cast(int)(val & 0x7)
  case .Global_Balance: psg.global_balance = cast(PSG_Balance)val
  case .Fine_Freq:
    if chan != nil {
      chan.freq &= 0xF00
      chan.freq |= cast(uint)val
    }
  case .Rough_Freq:
    if chan != nil {
      chan.freq &= 0xFF
      chan.freq |= (cast(uint)val&0xF)<<8
    }
  case .Chan_Flags:
    if chan != nil {
      if !chan.flags.on && chan.flags.dda_on {
        chan.write_idx = 0
      }

      chan.flags = cast(PSG_Chan_Flags)val
    }
  case .Chan_Balance:
    if chan != nil {
      chan.balance = cast(PSG_Balance)val
    }
  case .Chan_Sound_Data:
    if chan != nil {
      if !chan.flags.on && !chan.flags.dda_on {
        chan.samples[chan.write_idx] = transmute(PSG_Sample)(val&0x1F)
        chan.write_idx += 1
        chan.write_idx %= len(chan.samples)
      }
    }
  case .Noise_Enable_Freq:
    if chan != nil && (psg.chan_selected == 4 || psg.chan_selected == 5) {
      chan.noise = cast(PSG_Chan_Noise)val
    }
  case .Modulate_Freq: psg.lfo_freq = val
  case .Modulate_Flags: psg.lfo_flags = cast(PSG_LFO_Flags)val
  }
}
