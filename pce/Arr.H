#pragma once

#include "Basics.H"

template <class T, Uxx N>
struct Arr
{
  T data[N];

  constexpr inline
  T& operator[](Uxx i) {
    return data[i];
  }
};