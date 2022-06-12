#include <vimcat/version.h>
#include <stdbool.h>
#include "compiler.h"
#include <string.h>
#include <stddef.h>

extern const char *KNOWN_VERSIONS[] INTERNAL;
extern size_t KNOWN_VERSIONS_LENGTH INTERNAL;

bool vimcat_version_le(const char *v1, const char *v2) {

  // NULL is always incomparable
  if (v1 == NULL)
    return false;
  if (v2 == NULL)
    return false;

  // two identical versions are known equal
  if (strcmp(v1, v2) == 0)
    return true;

  // find the first version
  size_t v1_index;
  {
    bool found = false;
    for (size_t i = 0; i < KNOWN_VERSIONS_LENGTH; ++i) {
      if (strcmp(v1, KNOWN_VERSIONS[i]) == 0) {
        v1_index = i;
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  }

  // find the second version
  size_t v2_index;
  {
    bool found = false;
    for (size_t i = 0; i < KNOWN_VERSIONS_LENGTH; ++i) {
      if (strcmp(v2, KNOWN_VERSIONS[i]) == 0) {
        v2_index = i;
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  }

  return v1_index <= v2_index;
}
