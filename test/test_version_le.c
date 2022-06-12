// force assertions on
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#include <stddef.h>
#include <vimcat/vimcat.h>

int main(void) {

  // NULL should be incomparable to itself
  assert(!vimcat_version_le(NULL, NULL));
  assert(!vimcat_versions_comparable(NULL, NULL));
  assert(!vimcat_version_lt(NULL, NULL));
  assert(!vimcat_version_eq(NULL, NULL));
  assert(!vimcat_version_ne(NULL, NULL));
  assert(!vimcat_version_gt(NULL, NULL));

  // the oldest version we know
  const char first[] = "v2022.06.11";

  // NULL should be incomparable to a known version
  assert(!vimcat_version_le(first, NULL));
  assert(!vimcat_version_le(NULL, first));
  assert(!vimcat_versions_comparable(first, NULL));
  assert(!vimcat_version_lt(first, NULL));
  assert(!vimcat_version_eq(first, NULL));
  assert(!vimcat_version_ne(first, NULL));
  assert(!vimcat_version_gt(first, NULL));

  // an unknown version
  const char unknown[] = "foo bar";

  // NULL should also be incomparable to an known version
  assert(!vimcat_version_le(unknown, NULL));
  assert(!vimcat_version_le(NULL, unknown));
  assert(!vimcat_versions_comparable(unknown, NULL));
  assert(!vimcat_version_lt(unknown, NULL));
  assert(!vimcat_version_eq(unknown, NULL));
  assert(!vimcat_version_ne(unknown, NULL));
  assert(!vimcat_version_gt(unknown, NULL));

  // a known version should be equal to itself
  assert(vimcat_version_le(first, first));
  assert(vimcat_version_le(first, first));
  assert(vimcat_versions_comparable(first, first));
  assert(!vimcat_version_lt(first, first));
  assert(vimcat_version_eq(first, first));
  assert(!vimcat_version_ne(first, first));
  assert(!vimcat_version_gt(first, first));

  // an unknown version should also be equal to itself
  assert(vimcat_version_le(unknown, unknown));
  assert(vimcat_version_le(unknown, unknown));
  assert(vimcat_versions_comparable(unknown, unknown));
  assert(!vimcat_version_lt(unknown, unknown));
  assert(vimcat_version_eq(unknown, unknown));
  assert(!vimcat_version_ne(unknown, unknown));
  assert(!vimcat_version_gt(unknown, unknown));

  // a known version and an unknown version should be incomparable
  assert(!vimcat_version_le(unknown, first));
  assert(!vimcat_version_le(first, unknown));
  assert(!vimcat_versions_comparable(unknown, first));
  assert(!vimcat_version_lt(unknown, first));
  assert(!vimcat_version_eq(unknown, first));
  assert(!vimcat_version_ne(unknown, first));
  assert(!vimcat_version_gt(unknown, first));

  return 0;
}
