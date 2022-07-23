#include "help.h"
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vimcat/vimcat.h>

// enable colour highlighting?
static enum { ALWAYS, AUTO, NEVER } colour = AUTO;

static int print(void *ignored, const char *line) {

  (void)ignored;

  for (const char *p = line; *p != '\0'; ++p) {

    if (colour == NEVER) {
      // is this a Control Sequence Identifier?
      if (p[0] == '\033' && p[1] == '[') {
        // skip up until the terminator, if we can find one
        const char *q;
        for (q = p + 2; *q != '\0' && *q != 'm'; ++q)
          ;
        if (*q == 'm') {
          p = q;
          continue;
        }
      }
    }

    if (fputc(*p, stdout) == EOF)
      return errno;
  }

  // an empty line or the last file line may not be terminated, so ensure it
  // displays correctly
  if (strlen(line) == 0 || line[strlen(line) - 1] != '\n')
    putchar('\n');

  return 0;
}

int main(int argc, char **argv) {

  bool debug = false;

  while (true) {
    static const struct option opts[] = {
        {"color", required_argument, 0, 'c'},
        {"colour", required_argument, 0, 'c'},
        {"debug", no_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0},
    };

    int index = 0;
    int c = getopt_long(argc, argv, "c:dhv", opts, &index);

    if (c == -1)
      break;

    switch (c) {

    case 'c': // --colour
      if (strcmp(optarg, "always") == 0) {
        colour = ALWAYS;
      } else if (strcmp(optarg, "auto") == 0) {
        colour = AUTO;
      } else if (strcmp(optarg, "never") == 0) {
        colour = NEVER;
      } else {
        fprintf(stderr, "unrecognised option '%s' to --colour\n", optarg);
        return EXIT_FAILURE;
      }
      break;

    case 'd': // --debug
      debug = true;
      break;

    case 'h': // --help
      help();
      return EXIT_SUCCESS;

    case 'v': // --version
      printf("vimcat version %s\n", vimcat_version());
      return EXIT_SUCCESS;

    default:
      return EXIT_FAILURE;
    }
  }

  // check `$NO_COLOR` as a fallback mechanism
  if (colour == AUTO) {
    if (getenv("NO_COLOR") == NULL) {
      colour = ALWAYS;
    } else {
      if (debug)
        fprintf(stderr, "[VIMCAT] disabling colour output due to $NO_COLOR\n");
      colour = NEVER;
    }
  }

  if (debug)
    vimcat_debug_on();

  for (size_t i = optind; i < (size_t)argc; ++i) {
    int rc = vimcat_read(argv[i], print, NULL);
    if (rc != 0) {
      fprintf(stderr, "failed: %s\n", strerror(rc));
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
