#include "debug.h"
#include <stddef.h>
#include <stdio.h>
#include <vimcat/debug.h>

FILE *debug;

FILE *vimcat_set_debug(FILE *stream) {
  FILE *old = debug;
  debug = stream;
  return old;
}

void vimcat_debug_on(void) { (void)vimcat_set_debug(stderr); }

void vimcat_debug_off(void) { (void)vimcat_set_debug(NULL); }
