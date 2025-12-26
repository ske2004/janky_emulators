// @todo: emulate 9 cycle write delay
package main

import "core:fmt"

Joypad :: bit_field u8 {
  up:    bool | 1,
  right: bool | 1,
  down:  bool | 1,
  left:  bool | 1,
  a:     bool | 1,
  b:     bool | 1,
  sel:   bool | 1,
  run:   bool | 1,
}

Joy :: struct {
  high: bool,
  joypad: Joypad,
  joypad_idx: int,
}

joy_read :: proc(joy: ^Joy) -> u8 {
  if joy.joypad_idx == 0 || joy.joypad_idx == 1 {
    if joy.high {
      return (cast(u8)(joy.joypad)&0xF)~0xF
    } else {
      return (cast(u8)(joy.joypad)>>4)~0xF
    }
  }

  return 0
}

joy_write :: proc(joy: ^Joy, val: u8) {
  high := cast(bool)(val&1)
  clr := cast(bool)(val&2)

  if high != joy.high {
    joy.joypad_idx += 1
  }

  joy.high = high
  if clr {
    joy.joypad_idx = 0
  }
}