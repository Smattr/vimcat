#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VIMCAT_API
#define VIMCAT_API __attribute__((visibility("default")))
#endif

/** Vim-highlight the given file, returning lines through the callback function
 *
 * The callback function receives lines that have been syntax-highlighted using
 * ANSI terminal escape sequences. Iteration through the file will be terminated
 * when the end of file is reached or the caller’s callback returns non-zero.
 *
 * \param filename Source file to read
 * \param callback Handler for highlighted lines
 * \param state State to pass as first parameter to the callback
 * \returns 0 on success, an errno on failure, or the last non-zero return from
 *   the caller’s callback if there was one
 */
VIMCAT_API int vimcat_read(const char *filename,
                           int (*callback)(void *state, const char *line),
                           void *state);

#ifdef __cplusplus
}
#endif
