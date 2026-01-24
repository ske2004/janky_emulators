package main 

import "core:fmt"

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
  volume: uint | 5,
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
}

psg_get_chan :: proc(psg: ^PSG) -> ^PSG_Chan {
  if psg.chan_selected > 5 {
    return nil
  }

  return &psg.channels[psg.chan_selected]
}

psg_cycle_chan :: proc(psg: ^PSG, chan: ^PSG_Chan) {
  if psg.cycles % chan.freq*32 == 0 {
    chan.sample_idx += 1
    chan.sample_idx %= 32
  }
}

psg_read_sample :: proc(bus: ^Bus, psg: ^PSG, chan: uint) -> u8 {
  chan := psg.channels[chan]
  return chan.samples[chan.sample_idx]
}

psg_cycle_read_sample :: proc(bus: ^Bus, psg: ^PSG, chan: uint) -> (sample: u8) {
  cps := 3580000 / PSG_SAMPLE_RATE

  for i in 0..<cps {
    for &chan in psg.channels {
      psg_cycle_chan(psg, &chan)
    }

    psg.cycles += 1
  }

  if chan == 0 {
    t : uint = 0
    for chan, i in psg.channels {
      t += cast(uint)psg_read_sample(bus, psg, cast(uint)i)
    }

    sample = cast(u8)(t / 6)
  } else {
    sample = psg_read_sample(bus, psg, chan-1)
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
      chan.freq |= cast(uint)val<<8
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
        chan.samples[chan.write_idx] = val
        chan.write_idx += 1
        chan.write_idx %= len(chan.samples)
      }
    }
  case .Noise_Enable_Freq:
    if chan != nil {
      chan.noise = cast(PSG_Chan_Noise)val
    }
  case .Modulate_Freq: psg.lfo_freq = val
  case .Modulate_Flags: psg.lfo_flags = cast(PSG_LFO_Flags)val
  }
}
