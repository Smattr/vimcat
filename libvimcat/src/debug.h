#pragma once

#include "compiler.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern FILE *vimcat_debug INTERNAL;

/// emit a debug message
#define DEBUG(...)                                                             \
  do {                                                                         \
    if (UNLIKELY(vimcat_debug != NULL)) {                                      \
      const char *name_ = strrchr(__FILE__, '/');                              \
      flockfile(vimcat_debug);                                                 \
      fprintf(vimcat_debug, "[VIMCAT] libvimcat/src%s:%d: ", name_, __LINE__); \
      fprintf(vimcat_debug, __VA_ARGS__);                                      \
      fprintf(vimcat_debug, "\n");                                             \
      funlockfile(vimcat_debug);                                               \
    }                                                                          \
  } while (0)

/// logging wrapper for error conditions
#define ERROR(cond)                                                            \
  ({                                                                           \
    bool cond_ = (cond);                                                       \
    if (UNLIKELY(cond_)) {                                                     \
      int errno_ = errno;                                                      \
      DEBUG("`%s` failed (current errno is %d)", #cond, errno_);               \
      errno = errno_;                                                          \
    }                                                                          \
    cond_;                                                                     \
  })
