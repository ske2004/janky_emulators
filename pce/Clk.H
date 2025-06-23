/* Sync clock */
#pragma once

#include "Basics.H"
#include <functional>

using ClkAuxCycle = std::function<void()>;

struct Clk
{
  U64 counter=0;
  Bool halted=false;
  Bool ignoring=false;
  ClkAuxCycle aux_cycle; // aux cycle fn for simpler than CPU

  Clk(ClkAuxCycle aux_cycle) : aux_cycle(aux_cycle) {}
  auto Ignore(std::function<void()> cb)->void { ignoring=true; cb(); ignoring=false; }
  auto Halt()->void { halted = true; }
  auto Cycle()->void { if (!ignoring) {counter++; aux_cycle();} }
  auto IsGoing()->Bool { return !halted; }
};

