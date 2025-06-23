#include "Basics.H"

struct Str
{
  U8 *data;
  Uxx size;

  Str(const char *src)
  {
    size = strlen(src);
    new (char, 32)
  }

  ~Str()
  {
    delete data;
  }

  const char *C()
  {
    return data;
  }
};