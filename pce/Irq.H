#pragma once
#include "Basics.H"
#include "pce/Dbg.H"

struct Irq
{
  constexpr static U8 IRQ2 = 0x1;
  constexpr static U8 IRQ1 = 0x2;
  constexpr static U8 TIMER = 0x4;

  // assuming every interrupt is disabled
  // 1 = disabled >,>
  U8 enabled = 0b000;
  U8 pending = 0;

  void Ack(U8 which);
  bool Check(U8 irq) const;
  bool HasIrq() const;
  void Request(U8 irq);
};