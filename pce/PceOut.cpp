#include "PceOut.H"

static auto Rgb16ToRgba32(U16 rgb16) -> U32
{
  U32 r = (rgb16&0x7)<<5, g = ((rgb16>>3)&0x7)<<5, b = ((rgb16>>6)&0x7)<<5;
  return (r<<24) | (g<<16) | (b<<8);
}

auto PceOut::NextFrame() -> U0
{
  std::lock_guard l(mux);
  frame++;
}

auto PceOut::SetRGB16(U16 data[]) -> U0
{
  std::lock_guard l(mux);
  for (int i=0; i<PCE_SCREEN_H*PCE_SCREEN_W; i++) {
    screen[i] = Rgb16ToRgba32(data[i]);
  }
}