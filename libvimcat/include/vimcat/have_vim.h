#pragma once

#include <stdbool.h>

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

/** is `vim` available?
 *
 * \return True if libvimcat can find Vim
 */
VIMCAT_API bool vimcat_have_vim(void);

#ifdef __cplusplus
}
#endif
