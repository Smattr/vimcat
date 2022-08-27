#include <stdlib.h>
#include <vimcat/have_vim.h>

bool vimcat_have_vim(void) {
  return system("which vim >/dev/null 2>/dev/null") == EXIT_SUCCESS;
}
