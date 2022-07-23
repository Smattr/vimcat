#pragma once

#include "compiler.h"
#include <stdbool.h>
#include <stdint.h>

/// a 24-bit colour
typedef struct {
  uint8_t r; ///< red component
  uint8_t g; ///< green component
  uint8_t b; ///< blue component
} colour_t;

static inline bool colour_eq(colour_t a, colour_t b) {
  if (a.r != b.r)
    return false;
  if (a.g != b.g)
    return false;
  if (a.b != b.b)
    return false;
  return true;
}

/// convert an 8-bit colour to its 24-bit equuivalent
INTERNAL colour_t colour_8_to_24(uint8_t colour);

/// Convert a 24-bit colour to its 8-bit equivalent. Returns a value greater
/// than 255 if there is no equivalent.
INTERNAL unsigned colour_24_to_8(colour_t colour);
