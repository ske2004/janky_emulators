#include "third_party/pge/olcPixelGameEngine.h"
#include "Pce.H"
#include <mutex>

class PcPow : public olc::PixelGameEngine
{
  PceBin bin;
  PceOut out;
  std::thread pce_thread;

public:
  PcPow(CStr rom_path) : bin(rom_path)
  {
    sAppName = "PcPOW";
  }

  ~PcPow()
  {
    /* sucks */
    pce_thread.detach();
  }

  Bool OnUserCreate() override
  {
    pce_thread = std::thread([this]{
      Pce pce(bin);
      pce.Run(out);
    });

    return true;
  }

  Bool OnUserUpdate(F32 delta) override
  {
    U32 screen[PCE_SCREEN_W*PCE_SCREEN_H];
    {
      std::lock_guard l(out.mux);
      memcpy(screen, out.screen, sizeof(screen));
    }

    for (U32 x=0; x<PCE_SCREEN_W; x++) {
      for (U32 y=0; y<PCE_SCREEN_H; y++) {
        this->Draw(x, y, screen[x+y*PCE_SCREEN_W]);
      }
    }

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
    if (pcpow.Construct(PCE_SCREEN_W, PCE_SCREEN_H, 3, 3))
      pcpow.Start();
  } catch (CStr err) {
    Dbg::Fail("Fatal error: %s\n", err);
  }
}
