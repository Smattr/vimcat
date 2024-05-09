#include "help.h"
#include <errno.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __APPLE__
#include <crt_externs.h>
#endif

static char **get_environ(void) {
#ifdef __APPLE__
  // on macOS, environ is not directly accessible
  return *_NSGetEnviron();
#else
  // some platforms fail to expose environ in a header (e.g. FreeBSD), so
  // declare it ourselves and assume it will be available when linking
  extern char **environ;

  return environ;
#endif
}

// these symbols are generated by an xxd translation of vimcat.1
extern unsigned char vimcat_1[];
extern unsigned int vimcat_1_len;

// The approach we take below is writing the manpage to a temporary location and
// then asking man to display it. It would be nice to avoid the temporary file
// and just pipe the manpage to man on stdin. However, man on macOS does not
// seem to support reading from pipes. Since we need a work around for at least
// macOS, we just do it uniformly through a temporary file for all platforms.

int help(void) {

  int rc = 0;

  // find temporary storage space
  const char *TMPDIR = getenv("TMPDIR");
  if (TMPDIR == NULL)
    TMPDIR = "/tmp";

  // create a temporary path
  char *path = NULL;
  if (asprintf(&path, "%s/temp.XXXXXX", TMPDIR) < 0)
    return ENOMEM;

  // create a file there
  int fd = mkstemp(path);
  if (fd == -1) {
    rc = errno;
    goto done;
  }

  // write the manpage to the temporary file
  {
    ssize_t r = write(fd, vimcat_1, (size_t)vimcat_1_len);
    if (r < 0 || (size_t)r != vimcat_1_len) {
      rc = errno;
      goto done;
    }
  }

  // ensure the full content will be visible to subsequent readers
  (void)fsync(fd);

  // run man to display the help text
  pid_t man = 0;
  {
    const char *argv[] = {"man",
#ifdef __linux__
                          "--local-file",
#endif
                          path, NULL};
    char *const *args = (char *const *)argv;
    if ((rc = posix_spawnp(&man, argv[0], NULL, NULL, args, get_environ())))
      goto done;
  }

  // wait for man to finish
  (void)waitpid(man, (int[]){0}, 0);

  // cleanup
done:
  if (fd >= 0) {
    (void)close(fd);
    (void)unlink(path);
  }
  free(path);

  return rc;
}
