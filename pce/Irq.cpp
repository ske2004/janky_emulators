#include "Irq.H"

void Irq::Ack(U8 which)
{
  // acknowledge everything? idk
  pending &= ~which;
}

bool Irq::Check(U8 irq) const
{
  return (pending & irq) > 0;
}

bool Irq::HasIrq() const
{
  return pending & 0x7;
}

void Irq::Request(U8 irq)
{
  if ((enabled & irq) == 0) {
    Dbg::Info("IRQ", "Requesting IRQ $%d", irq);
    pending |= irq;
  }
}