#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VIMCAT_API
#define VIMCAT_API __attribute__((visibility("default")))
#endif

/** retrieve version of this library
 *
 * \return A version string
 */
VIMCAT_API const char *vimcat_version(void);

/** compare one Vimcat version string to another
 *
 * Comparison between two versions can have four possible answers:
 *
 *   1. `v1` is older than `v2`
 *   2. `v1` is the same as `v2`
 *   3. `v1` is newer than `v2`
 *   4. `v1` and `v2` are incomparable
 *
 * Answer 4 is a possibility because either or both of the versions could be a
 * future release unknown to the current implementation or a non-release version
 * built from source. In both of these scenarios, the library is unable to order
 * the two versions.
 *
 * So to compare two versions you most likely need two calls to this function:
 *
 *   1. v1 ≤ v2, `vimcat_version_le(v1, v2)`
 *   2. v2 ≤ v1, `vimcat_version_le(v2, v1)`
 *
 * the answers to which tell you:
 *
 *   ╭───────────────╥──────────────┬───────────────╮
 *   │               ║ v1 ≤ v2 true │ v1 ≤ v2 false │
 *   ╞═══════════════╬══════════════╪═══════════════╡
 *   │ v2 ≤ v1 true  ║   v1 == v2   │    v1 > v2    │
 *   ├───────────────╫──────────────┼───────────────┤
 *   │ v2 ≤ v1 false ║   v1 < v2    │  incomparable │
 *   ╰───────────────╨──────────────┴───────────────╯
 *
 * Wrappers below do this for you.
 *
 * \return True if `v1` is older than or the same as `v2`
 */
VIMCAT_API bool vimcat_version_le(const char *v1, const char *v2);

static inline bool vimcat_versions_comparable(const char *v1, const char *v2) {
  return vimcat_version_le(v1, v2) || vimcat_version_le(v2, v1);
}

static inline bool vimcat_version_lt(const char *v1, const char *v2) {
  return vimcat_version_le(v1, v2) && !vimcat_version_le(v2, v1);
}

static inline bool vimcat_version_eq(const char *v1, const char *v2) {
  return vimcat_version_le(v1, v2) && vimcat_version_le(v2, v1);
}

static inline bool vimcat_version_ne(const char *v1, const char *v2) {
  return (vimcat_version_le(v1, v2) && !vimcat_version_le(v2, v1)) ||
         (!vimcat_version_le(v1, v2) && vimcat_version_le(v2, v1));
}

static inline bool vimcat_version_gt(const char *v1, const char *v2) {
  return !vimcat_version_le(v1, v2) && vimcat_version_le(v2, v1);
}

#ifdef __cplusplus
}
#endif
