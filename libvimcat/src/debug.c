#include "debug.h"
#include <stddef.h>
#include <stdio.h>
#include <vimcat/debug.h>

FILE *vimcat_debug;

FILE *vimcat_set_debug(FILE *stream) {
  FILE *old = vimcat_debug;
  vimcat_debug = stream;
  return old;
}

void vimcat_debug_on(void) { (void)vimcat_set_debug(stderr); }

void vimcat_debug_off(void) { (void)vimcat_set_debug(NULL); }
