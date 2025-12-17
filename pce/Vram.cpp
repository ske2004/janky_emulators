#pragma once

#include "Vram.H"

void VramSprite::Size(U16 &w, U16 &h)
{
  w = 16;
  h = 16;

  if (flags&FLAG_XS) {
    w = 32;
  }

  if (flags&FLAG_H1) {
    h = 32;
  }

  if (flags&FLAG_H2) {
    h = 64;
  }
}

auto Vram::GetSprite(Uxx which) -> VramSprite
{
  return *(VramSprite*)(&sprites[which*4]);
}