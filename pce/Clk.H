/* Sync clock */
#pragma once

#include "Basics.H"

using ClkAuxCycle = void (*)(void *);

struct Clk
{
  U64 counter;
  Bool halted;
  void *userdata;
  ClkAuxCycle AuxCycle; // aux cycle fn for simpler than CPU
};

auto ClkNew(void *userdata, ClkAuxCycle AuxCycle) -> Clk
{
  Clk clk = {};
  clk.AuxCycle = AuxCycle;
  return clk;
}

auto ClkHalt(Clk &clk) -> void
{
  clk.halted = true;
}

auto ClkCycle(Clk &clk) -> void
{
  clk.counter++;
  clk.AuxCycle(clk.userdata);
}

auto ClkIsGoing(Clk &clk) -> bool
{
  return !clk.halted;
}
