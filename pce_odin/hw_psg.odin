package main 

import "core:fmt"
import "core:math/rand"

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

PSG_Chan :: struct {
  samples:    [32]u8,
  freq:       uint,
  flags:      PSG_Chan_Flags,
  balance:    PSG_Balance,
  noise:      PSG_Chan_Noise,
  sample_idx: uint,
  write_idx:  uint,
  
  _noise_state: u8
}

PSG :: struct {
  ring_buf:       [1024]u8,
  ring_read_idx:  uint,
  ring_write_idx: uint,
  chan_selected:  int,
  channels:       [6]PSG_Chan,
  global_balance: PSG_Balance,
  lfo_freq:       u8,
  lfo_flags:      PSG_LFO_Flags,
  cycles:         uint,
  rng:            rand.Xoshiro256_Random_State
}

psg_get_chan :: proc(psg: ^PSG) -> ^PSG_Chan {
  if psg.chan_selected > 5 {
    return nil
  }

  return &psg.channels[psg.chan_selected]
}

psg_cycle_chan :: proc(psg: ^PSG, chan: ^PSG_Chan) {
  if chan.noise.on {
    t := (chan.noise.freq~0x1F)*2
    if t != 0 && psg.cycles%t == 0 {
      if chan.sample_idx == 0 {
        chan._noise_state = cast(u8)(rand.uint_max(2, rand.xoshiro256_random_generator(&psg.rng)))*31
        fmt.printf("Noise state: %v\n", chan._noise_state)
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

psg_read_sample :: proc(bus: ^Bus, psg: ^PSG, chan: uint) -> u8 {
  chan := psg.channels[chan]
  if chan.noise.on {
    return chan._noise_state
  } else {
    return chan.samples[chan.sample_idx]
  }
}

psg_cycle_read_sample :: proc(bus: ^Bus, psg: ^PSG, chan: uint) -> (l: u8, r: u8) {
  cps := 3580000 / PSG_SAMPLE_RATE

  if chan == 0 {
    lt, rt: f32 = 0, 0
    for chan, i in psg.channels {
      if chan.flags.on {
        sample := cast(f32)psg_read_sample(bus, psg, cast(uint)i)
        lt += sample*
              (cast(f32)chan.flags.volume/32.0)*
              (cast(f32)chan.balance.left/16.0);
        rt += sample*
              (cast(f32)chan.flags.volume/32.0)*
              (cast(f32)chan.balance.right/16.0);
      }
    }

    lt *= (cast(f32)psg.global_balance.left/16.0)
    rt *= (cast(f32)psg.global_balance.right/16.0)

    l = cast(u8)(lt / cast(f32)len(psg.channels))
    r = cast(u8)(rt / cast(f32)len(psg.channels))
  } else {
    l = psg_read_sample(bus, psg, chan-1)
    r = l
  }

  for i in 0..<cps {
    for &chan in psg.channels {
      psg_cycle_chan(psg, &chan)
    }

    psg.cycles += 1
  }

  return 
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
      chan.flags = cast(PSG_Chan_Flags)val
    }
  case .Chan_Balance:
    if chan != nil {
      chan.balance = cast(PSG_Balance)val
    }
  case .Chan_Sound_Data:
    if chan != nil {
      if !chan.flags.on && !chan.flags.dda_on {
        chan.samples[chan.write_idx] = val&0x1F
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
