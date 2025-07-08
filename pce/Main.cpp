#include "Pce.H"
#include "third_party/pge/olcPixelGameEngine.h"

class PcPow : public olc::PixelGameEngine
{
  PceBin bin;
  Pce pce;

public:
  PcPow(CStr rom_path) : bin(rom_path), pce(bin)
  {
    sAppName = "PcPOW";
  }

  Bool OnUserCreate() override
  {
    return true;
  }

  Bool OnUserUpdate(F32 delta) override
  {
    return true;
  }
};

auto main(I32 argc, CStr argv[]) -> I32
{
  if (argc != 2) {
    printf("%s <rom_file>\n", argv[0]);
    return 1;
  }

  Dbg::Info("MAIN: Loading ROM %s", argv[1]);

  try {
    PcPow pcpow(argv[1]);
    if (pcpow.Construct(256, 224, 3, 3))
      pcpow.Start();
  } catch (CStr err) {
    Dbg::Fail("Fatal error: %s\n", err);
  }
}
