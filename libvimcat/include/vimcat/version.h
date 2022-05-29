#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VIMCAT_API
#define VIMCAT_API __attribute__((visibility("default")))
#endif

/** retrieve version of this library
 *
 * For now, the version of Vimcat is an opaque string. You cannot use it to
 * determine whether one version is newer than another. The most you can do is
 * `strcmp` two version strings to determine if they are the same.
 *
 * \return A version string
 */
VIMCAT_API const char *vimcat_version(void);

#ifdef __cplusplus
}
#endif
