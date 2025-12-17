#include "Clk.H"

auto Clk::Ignore(std::function<void()> cb)->void
{
  ignoring=true;
  cb();
  ignoring=false;
}

auto Clk::Halt()->void
{
  halted = true;
}

auto Clk::ExtraCycle()->void
{
  if (!ignoring) {
    extra_counter++;
    counter++;
    aux_cycle();
  }
}

auto Clk::Cycle()->void
{
  if (!ignoring) {
    counter++;
    aux_cycle();
  }
}

auto Clk::IsGoing()->Bool
{
  return !halted;
}