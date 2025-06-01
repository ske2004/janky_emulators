#include "Basics.H"
#include "Bin.H"
#include <cstdio>

struct Pce
{
  Bin *rom;
  U8 ram[0x2000];
};

auto PceNew(Bin *rom) -> Pce
{
  auto pce = Pce();
  pce.rom = rom;
  return pce;
}

auto main(I32 argc, CStr argv[]) -> I32
{
  if (argc != 2) {
    printf("%s <rom_file>\n", argv[0]);
    return 1;
  }

  Bin *rom = BinRead(argv[1]);
  Pce pce = PceNew(rom);
  BinDel(rom);
}
