#pragma once

#include "compiler.h"

/** common logic of `vimcat_read` and `vimcat_read_line`
 *
 * \param filename Source file to read
 * \param lineno Line number to highlight, or 0 to highlight all lines
 * \param callback Handler for highlighted line(s)
 * \param state State to pass as first parameter to the callback
 * \returns 0 on success, an errno on failure, or the last non-zero return from
 *   the caller’s callback if there was one
 */
INTERNAL int read_core(const char *filename, unsigned long lineno,
                       int (*callback)(void *state, const char *line),
                       void *state);
