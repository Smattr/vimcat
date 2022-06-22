#pragma once

#include "compiler.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern FILE *vimcat_debug INTERNAL;

/// emit a debug message
#define DEBUG(args...)                                                         \
  do {                                                                         \
    if (UNLIKELY(vimcat_debug != NULL)) {                                      \
      const char *name_ = strrchr(__FILE__, '/');                              \
      flockfile(vimcat_debug);                                                 \
      fprintf(vimcat_debug, "[VIMCAT] libvimcat/src%s:%d: ", name_, __LINE__); \
      fprintf(vimcat_debug, args);                                             \
      fprintf(vimcat_debug, "\n");                                             \
      funlockfile(vimcat_debug);                                               \
    }                                                                          \
  } while (0)
