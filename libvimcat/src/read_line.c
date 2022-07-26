#include "debug.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <vimcat/read.h>

// TODO: the implementation below could be significantly accelerated by
// rendering a version of the file scrolled to the given line to a 2-row virtual
// window, instead of calling the generic highlight function

/// a sentinel value chosen to not collide with any errno
static const int DONE = -42;

/// state used in callback below
typedef struct {
  const unsigned long lineno; ///< sought after line
  unsigned long current;      ///< number of current line number we see
  char *result;               ///< retrieved line on success
} state_t;

static int find_line(void *state, const char *line) {

  assert(state != NULL);
  assert(line != NULL);

  state_t *s = state;
  ++s->current;
  assert(s->current <= s->lineno);

  assert(s->result == NULL);

  // is this the line we are searching for?
  if (s->current == s->lineno) {
    s->result = strdup(line);
    if (ERROR(s->result == NULL))
      return ENOMEM;
    return DONE;
  }

  return 0; // keep going
}

int vimcat_read_line(const char *filename, unsigned long lineno, char **line) {

  if (ERROR(filename == NULL))
    return EINVAL;

  if (ERROR(lineno == 0))
    return EINVAL;

  if (ERROR(line == NULL))
    return EINVAL;

  // highlight the file
  state_t s = {.lineno = lineno};
  int rc = vimcat_read(filename, find_line, &s);

  // was the line number sought was greater than the total lines in the file?
  if (rc == 0) {
    assert(s.result == NULL);
    return ERANGE;
  }

  // did an error occur during highlighting?
  if (rc != DONE) {
    assert(s.result == NULL);
    return rc;
  }

  *line = s.result;
  return 0;
}
