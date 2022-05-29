/// \file
/// \brief debugging functions
///
/// Applications should include the general API header, vimcat.h, in preference
/// to selectively including this.

#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VIMCAT_API
#define VIMCAT_API __attribute__((visibility("default")))
#endif

/** set destination for debug messages
 *
 * On startup, debug messages are suppressed. This function (or one of its
 * alternatives below) must be called to enable debugging.
 *
 * \param The stream to write debug messages to or `NULL` to suppress debugging
 *   output
 * \returns The previous stream set for debug messages
 */
VIMCAT_API FILE *vimcat_set_debug(FILE *stream);

/** enable debug messages to stderr
 *
 * This is a shorthand for `(void)vimcat_set_debug(stderr)`.
 */
VIMCAT_API void vimcat_debug_on(void);

/** disable debug messages
 *
 * This is a shorthand for `(void)vimcat_set_debug(NULL)`.
 */
VIMCAT_API void vimcat_debug_off(void);

#ifdef __cplusplus
}
#endif
