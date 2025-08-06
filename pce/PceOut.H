#include <mutex>
#include "Basics.H"

#define PCE_SCREEN_W 256
#define PCE_SCREEN_H 224

struct PceOut
{
  std::mutex mux;
  U32 screen[PCE_SCREEN_H*PCE_SCREEN_W];
};
