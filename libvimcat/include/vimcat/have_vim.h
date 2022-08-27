#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VIMCAT_API
#define VIMCAT_API __attribute__((visibility("default")))
#endif

/** is `vim` available?
 *
 * \return True if libvimcat can find Vim
 */
VIMCAT_API bool vimcat_have_vim(void);

#ifdef __cplusplus
}
#endif
