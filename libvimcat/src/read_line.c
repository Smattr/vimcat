#include "debug.h"
#include "read_core.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <vimcat/read.h>

static int accept_line(void *state, char *line) {

  assert(state != NULL);
  assert(line != NULL);

  // save the line we received
  char **result = state;
  *result = strdup(line);
  if (ERROR(*result == NULL))
    return ENOMEM;

  return 0;
}

int vimcat_read_line(const char *filename, unsigned long lineno, char **line) {

  if (ERROR(filename == NULL))
    return EINVAL;

  if (ERROR(lineno == 0))
    return EINVAL;

  if (ERROR(line == NULL))
    return EINVAL;

  // highlight the line in the file
  return read_core(filename, lineno, accept_line, line);
}
