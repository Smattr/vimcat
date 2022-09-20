#include "help.h"
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vimcat/vimcat.h>

// enable colour highlighting?
static enum { ALWAYS, AUTO, NEVER } colour = AUTO;

static int print(void *ignored, char *line) {

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

/// scary text to be shown to users on first run
static const char RIOT_ACT[] =
    "${HOME}/.vimcatrc not found; aborting\n"
    "\n"
    "To display a file as Vim would, Vimcat runs Vim as a subprocess. This\n"
    "means your ~/.vimrc will be evaluated along with any plugins you have\n"
    "enabled, custom syntax, etc. This can be surprising to new users. Some\n"
    "Vim features can even enable arbitrary code execution, whether\n"
    "intentionally or unintentionally (e.g. CVE-2002-1377, CVE-2016-1248,\n"
    "CVE-2019-12735). So running Vimcat on an untrusted file has some\n"
    "associated risk.\n"
    "\n"
    "To acknowledge that you understand these risks and want to run Vimcat,\n"
    "create a text file .vimcatrc in your ${HOME} directory.\n";

/// check the user has indicated they have read the riot act
static void check_consent(void) {

  bool have_vimcatrc = false;

  const char *home = getenv("HOME");
  if (home != NULL) {
    char *vimcatrc = NULL;
    if (asprintf(&vimcatrc, "%s/.vimcatrc", home) < 0) {
      fprintf(stderr, "out of memory\n");
      exit(EXIT_FAILURE);
    }
    have_vimcatrc = access(vimcatrc, F_OK) == 0;
    free(vimcatrc);
  }

  if (!have_vimcatrc) {
    fputs(RIOT_ACT, stderr);
    exit(EXIT_FAILURE);
  }
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

  check_consent();

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

  if (!vimcat_have_vim()) {
    fprintf(stderr, "vim not found\n");
    return EXIT_FAILURE;
  }

  for (size_t i = optind; i < (size_t)argc; ++i) {
    int rc = vimcat_read(argv[i], print, NULL);
    if (rc != 0) {
      fprintf(stderr, "failed: %s\n", strerror(rc));
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
