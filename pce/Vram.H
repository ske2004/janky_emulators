#pragma once

#include "Basics.H"

struct VramSprite
{
  defexpr U16 FLAG_FG =   0x80; // is on foreground?
  defexpr U16 FLAG_XS =  0x100; // is double size (TODO: more tiles or stretched?)
  defexpr U16 FLAG_XF =  0x800; // x flip
  defexpr U16 FLAG_H1 = 0x1000; // 
  defexpr U16 FLAG_H2 = 0x2000; // y size (2 fields this breaks consistency :c)
  defexpr U16 FLAG_YF = 0x8000; // y flip

  void Size(U16 &w, U16 &h);

  U16 y; // 10 bit
  U16 x; // 10 bit
  U16 tile; // 11 bit
  U16 flags;
};

struct Vram
{
  U16 vram[0x8000] {};
  U16 sprites[0x100] {};

  auto GetSprite(Uxx which) -> VramSprite;
};