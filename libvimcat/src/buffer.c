#include "buffer.h"
#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int buffer_open(buffer_t *b) {
  assert(b != NULL);

  *b = (buffer_t){0};

  b->f = open_memstream(&b->base, &b->size);
  if (ERROR(b->f == NULL))
    return errno;

  return 0;
}

void buffer_sync(buffer_t *b) {
  assert(b != NULL);
  assert(b->f != NULL);

  int rc = fflush(b->f);
  assert(rc == 0);
  (void)rc;
}

void buffer_clear(buffer_t *b) {
  assert(b != NULL);
  assert(b->f != NULL);

  buffer_sync(b);
  memset(b->base, 0, b->size);
  rewind(b->f);
}

void buffer_close(buffer_t *b) {

  if (b == NULL)
    return;

  if (b->f != NULL)
    (void)fclose(b->f);

  free(b->base);

  *b = (buffer_t){0};
}
