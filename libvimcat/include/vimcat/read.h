#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VIMCAT_API
#ifdef __GNUC__
#define VIMCAT_API __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#define VIMCAT_API __declspec(dllexport)
#else
#define VIMCAT_API /* nothing */
#endif
#endif

/** Vim-highlight the given file, returning lines through the callback function
 *
 * The callback function receives lines that have been syntax-highlighted using
 * ANSI terminal escape sequences. Iteration through the file will be terminated
 * when the end of file is reached or the caller’s callback returns non-zero.
 *
 * The callback should not free the \p line passed to it, but it is free to
 * modify the pointed to data. \p line is only valid until \p callback returns.
 * After that, the underlying memory may be repurposed.
 *
 * \param filename Source file to read
 * \param callback Handler for highlighted lines
 * \param state State to pass as first parameter to the callback
 * \return 0 on success, an errno on failure, or the last non-zero return from
 *   the caller’s callback if there was one
 */
VIMCAT_API int vimcat_read(const char *filename,
                           int (*callback)(void *state, char *line),
                           void *state);

/** Vim-highlight a single line in the given file
 *
 * This function provides a convenience one-shot version of `vimcat_read` for
 * callers who do not need the contents of the entire file.
 *
 * \param filename Source file to read
 * \param lineno Line number of the line to highlight
 * \param [out] line Highlighted line on success
 * \return 0 on success or an errno on failure
 */
VIMCAT_API int vimcat_read_line(const char *filename, unsigned long lineno,
                                char **line);

#ifdef __cplusplus
}
#endif
