#include "compiler.h"
#include "debug.h"
#include "get_environ.h"
#include "read_core.h"
#include "term.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vimcat/read.h>

// To understand the code that follows, it is useful to know several
// characteristics of how Vim renders a file to the terminal:
//
//   1. The content of the file itself is preceded by a status line. At the end
//      of this, Vim emits "\033[1;1H" to move back to the top left to begin
//      emitting the file content.
//
//   2. If Vim needs to wrap a line, it will emit "\033[<line>;1H" midway
//      through a line to adjust the cursor position. If we fake large enough
//      terminal dimensions to Vim, we should never see these sequences, except
//      as noted in (3).
//
//   3. A run of blank lines is emitted by Vim as a single "\033[<line>;1H" to
//      move the cursor across this range.
//
//   4. Formatting state is not always reset at the end of a line. That is, if
//      a given line changed colours or style (bold, italic, …), the line may
//      not end in "\033[0m" if the next line begins in the same colouring and
//      style.
//
//   5. The content of the file is followed by a series of mode adjusting
//      sequences (optionally enabling mouse events, changing repainting, …).
//      These are mostly from the “private sequences” space of ANSI escape
//      codes.
//
//   6. Trailing blank lines in the file are not emitted by Vim at all, as they
//      do not need display.

/// learn the number of lines and maximum line width of a text file
static int get_extent(const char *filename, size_t limit, size_t *rows,
                      size_t *columns) {
  assert(filename != NULL);

  FILE *f = fopen(filename, "r");
  if (ERROR(f == NULL))
    return errno;

  size_t lines = 1;
  size_t width = 0;
  size_t max_width = 0;
  int last = EOF;

  int rc = 0;

  while (true) {

    // have we scanned as far as the caller requested?
    if (limit != 0 && lines > limit)
      break;

    int c = getc(f);
    if (c == EOF)
      break;
    last = c;

    // is this a Unix end of line?
    if (c == '\n') {
      ++lines;
      if (max_width < width)
        max_width = width;
      width = 0;
      continue;
    }

    // is this a Windows end of line?
    if (c == '\r') {
      int n = getc(f);
      if (n == '\n') {
        ++lines;
        if (max_width < width)
          max_width = width;
        width = 0;
        continue;
      }

      if (n != EOF) {
        if (ERROR(ungetc(n, f) == EOF)) {
          rc = EIO;
          goto done;
        }
      }
    }

    // assume a tab stop is ≤ 8 characters
    if (c == '\t') {
      width += 8;
      continue;
    }

    // Anything else counts as a single character. This over-counts the width
    // required for UTF-8 characters, but that is fine.
    ++width;
  }

  if (max_width < width)
    max_width = width;

  // if the file ended with a newline, we do not count the next (empty) line
  if (last == '\n') {
    assert(lines > 1);
    --lines;
  }

done:
  (void)fclose(f);

  if (LIKELY(rc == 0)) {
    *rows = lines;
    *columns = max_width;
  }

  return rc;
}

/// start Vim, reading and displaying the given file at the given dimensions
static int run_vim(FILE **out, pid_t *pid, const char *filename, size_t rows,
                   size_t columns, size_t top_row) {

  assert(out != NULL);
  assert(pid != NULL);
  assert(filename != NULL);
  assert(columns >= 80 && "missing min clamping in vimcat_read?");
  assert(columns <= 10000 && "Vim will not render this many columns");
  assert(rows >= 1 && "missing min clamping in vimcat_read?");
  assert(rows <= 1000 && "Vim will not render this many rows");

  // clamp the top row to a legal value
  if (top_row == 0)
    top_row = 1;

  int rc = 0;
  FILE *output = NULL;
  int devnull = -1;

  posix_spawn_file_actions_t actions;
  if (ERROR((rc = posix_spawn_file_actions_init(&actions))))
    return rc;

  // create a pipe on which we can receive Vim’s rendering of the file
  int fd[2] = {-1, -1};
  if (ERROR(pipe(fd) < 0)) {
    rc = errno;
    goto done;
  }

  // set close-on-exec on the read end which the child (Vim) does not need
  {
    int flags = fcntl(fd[0], F_GETFD);
    if (ERROR(fcntl(fd[0], F_SETFD, flags | O_CLOEXEC) == -1)) {
      rc = errno;
      goto done;
    }
  }

  // turn the read end of the pipe into a file handle
  output = fdopen(fd[0], "r");
  if (ERROR(output == NULL)) {
    rc = errno;
    goto done;
  }
  fd[0] = -1;

  // dup the write end of the pipe over Vim’s stdout
  if (ERROR((rc = posix_spawn_file_actions_adddup2(&actions, fd[1],
                                                   STDOUT_FILENO))))
    goto done;

  // dup /dev/null over Vim’s stdin and stderr
  devnull = open("/dev/null", O_RDWR);
  if (ERROR(devnull < 0)) {
    rc = errno;
    goto done;
  }
  if (ERROR((rc = posix_spawn_file_actions_adddup2(&actions, devnull,
                                                   STDIN_FILENO))))
    goto done;
  if (ERROR((rc = posix_spawn_file_actions_adddup2(&actions, devnull,
                                                   STDERR_FILENO))))
    goto done;

  // construct Vim parameter to force terminal height
  char set_rows[sizeof("+set lines=") + 20];
  (void)snprintf(set_rows, sizeof(set_rows), "+set lines=%zu", rows);

  // construct Vim parameter to force terminal width
  char set_columns[sizeof("+set columns=") + 20];
  (void)snprintf(set_columns, sizeof(set_columns), "+set columns=%zu", columns);

  // prefix of the command we will run
  enum { ARGS = 17 };
  char const *argv[ARGS] = {
      "vim",
      "-R",                // read-only mode
      "--not-a-term",      // do not check whether std* is a TTY
      "-X",                // do not connect to X server
      "+set nonumber",     // hide line numbers in case the user has them on
      "+set laststatus=0", // hide status footer line
      "+set noruler",      // hide row,column position footer
      "+set nowrap",       // disable text wrapping in case we have long rows
      "+set scrolloff=0",  // make `z<CR>` scroll cursor row to the top
      set_rows,
      set_columns,
  };
  size_t arg_index = 0;
  while (argv[arg_index] != NULL) {
    ++arg_index;
    assert(arg_index < ARGS);
  }

#define APPEND(str)                                                            \
  do {                                                                         \
    assert(argv[arg_index] == NULL && "overwriting existing argument");        \
    argv[arg_index] = (str);                                                   \
    ++arg_index;                                                               \
    assert(arg_index < ARGS && "exceeding allocated Vim arguments");           \
  } while (0)

  // if we need to jump to a later row, construct Vim parameters to move there
  // and scroll the window such that this line is at the top
  char jump[sizeof("+normal! Gz\r") + 20];
  if (top_row > 1) {
    (void)snprintf(jump, sizeof(jump), "+normal! %zuGz\r", top_row);

    APPEND(jump);

    DEBUG("running Vim with '+set lines=%zu', '+set columns=%zu', '+normal! "
          "%zuGz<CR>' on %s",
          rows, columns, top_row, filename);
  } else {
    DEBUG("running Vim with '+set lines=%zu', '+set columns=%zu' on %s", rows,
          columns, filename);
  }

  APPEND("+redraw"); // force a screen render to happen before exiting
  APPEND("+qa!");    // exit with prejudice
  APPEND("--");
  APPEND(filename);

#undef APPEND

#ifndef NDEBUG
  {
    size_t commands = 0;
    for (size_t i = 0; i < sizeof(argv) / sizeof(argv[0]); ++i) {
      assert(argv[i] != NULL);
      if (argv[i][0] == '+')
        ++commands;
      if (strcmp(argv[i], "--") == 0)
        break;
    }
    assert(commands <= 10 && "too many commands for Vim to handle");
  }
#endif

  // spawn Vim
  pid_t p = 0;
  if (ERROR(((rc = posix_spawnp(&p, argv[0], &actions, NULL,
                                (char *const *)argv, get_environ())))))
    goto done;
  DEBUG("vim is PID %ld", (long)p);

  // success
  *out = output;
  output = NULL;
  *pid = p;

done:
  if (devnull >= 0)
    (void)close(devnull);
  if (output != NULL)
    (void)fclose(output);
  if (fd[0] >= 0)
    (void)close(fd[0]);
  if (fd[1] >= 0)
    (void)close(fd[1]);
  (void)posix_spawn_file_actions_destroy(&actions);

  return rc;
}

int read_core(const char *filename, unsigned long lineno,
              int (*callback)(void *state, const char *line), void *state) {

  assert(filename != NULL);
  assert(callback != NULL);

  int rc = 0;
  term_t *term = NULL;

  // learn the extent (character width and height) of this file so we can lie to
  // Vim and claim we have a terminal of these dimensions to prevent it
  // line-wrapping and/or truncating
  size_t rows = 0;
  size_t columns = 0;
  if (ERROR((rc = get_extent(filename, (size_t)lineno, &rows, &columns))))
    goto done;

  DEBUG("%s has %zu rows and %zu columns", filename, rows, columns);

  // was the requested line beyond the extent of the file?
  if (ERROR(lineno != 0 && (size_t)lineno > rows)) {
    rc = ERANGE;
    goto done;
  }

  size_t term_rows = rows;
  size_t term_columns = columns;

  // we only need a single row if we are highlighting one line
  if (lineno > 0) {
    rows = (size_t)lineno;
    term_rows = 1;
  }

  // we need one extra row for the Vim statusline
  ++term_rows;

  // bump the terminal dimensions if they are likely to confuse or impede Vim
  if (term_rows < 2) {
    DEBUG("clamping terminal rows from %zu to 2", term_rows);
    term_rows = 2;
  }
  if (term_columns < 80) {
    DEBUG("clamping terminal columns from %zu to 80", term_columns);
    term_columns = 80;
  }

  // Vim has a hard limit of 10000 columns, so if the file is wider than that we
  // just let anything beyond this be invisible
  if (UNLIKELY(term_columns > 10000)) {
    DEBUG("clamping terminal columns from %zu to 10000", term_columns);
    term_columns = 10000;
  }

  // Vim has a hard limit of 1000 rows, so subtract 1 for the statusline and
  // move in chunks of 999 rows if we have a file taller than this
  if (term_rows > 1000) {
    DEBUG("clamping terminal rows from %zu to 1000", term_rows);
    term_rows = 1000;
  }

  // create a virtual terminal
  if (ERROR((rc = term_new(&term, term_columns, term_rows))))
    goto done;

  for (size_t row = lineno == 0 ? 1 : (size_t)lineno; row <= rows;) {

    // if we are beyond the first iteration of this loop, clear terminal
    // contents from the last iteration
    if (lineno == 0 && row > 1)
      term_reset(term);

    // how many rows can we render in this pass?
    size_t vim_rows = rows - row + 1;
    if (vim_rows > 999)
      vim_rows = 999;

    // ask Vim to render the file
    FILE *vim_stdout = NULL;
    pid_t vim = 0;
    if (ERROR((rc = run_vim(&vim_stdout, &vim, filename, term_rows,
                            term_columns, row))))
      goto done;

    assert(vim_stdout != NULL && "invalid stream for Vim’s output");
    assert(vim > 0 && "invalid PID for Vim");

    // drain Vim’s output into the virtual terminal
    rc = term_send(term, vim_stdout);

    // if we failed to drain the entire output, discard the rest now
    if (ERROR(rc != 0)) {
      while (getc(vim_stdout) != EOF)
        ;
    }

    // clean up after Vim
    {
      (void)fclose(vim_stdout);
      vim_stdout = NULL;

      DEBUG("waiting for Vim to exit...");
      int status;
      if (ERROR(waitpid(vim, &status, 0) < 0)) {
        if (rc == 0) {
          rc = errno;
          DEBUG("waitpid failed: %s", strerror(rc));
        }
      } else if (rc == 0) {
        if (WIFEXITED(status)) {
          rc = WEXITSTATUS(status);
          if (UNLIKELY(rc != 0))
            DEBUG("Vim exited with failure: %d", rc);
        } else {
          rc = status;
          DEBUG("Vim exited abnormally: %d", rc);
        }
      }
      vim = 0;
      if (UNLIKELY(rc != 0))
        goto done;
    }

    // pass terminal lines back to the caller
    for (size_t y = 1; y <= vim_rows; ++y) {

      const char *line = NULL;
      if (ERROR((rc = term_readline(term, y, &line))))
        goto done;

      if (UNLIKELY((rc = callback(state, line))))
        goto done;
    }

    row += vim_rows;
  }

done:
  term_free(&term);

  return rc;
}

int vimcat_read(const char *filename,
                int (*callback)(void *state, const char *line), void *state) {

  if (ERROR(filename == NULL))
    return EINVAL;

  if (ERROR(callback == NULL))
    return EINVAL;

  return read_core(filename, 0, callback, state);
}
