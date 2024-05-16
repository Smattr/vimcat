#!/usr/bin/env python3

"""
Generate contents of a version.c.
"""

import os
from pathlib import Path
import re
import shutil
import subprocess as sp
import sys
from typing import Iterator, Optional

def all_versions() -> Iterator[str]:
  """
  All known versions in the CHANGELOG.rst.
  """
  with open(Path(__file__).parent / "../../CHANGELOG.rst", "rt") as f:
    for line in f:
      m = re.match(r"(v\d{4}\.\d{2}\.\d{2})$", line)
      if m is not None:
        yield m.group(1)

def last_release() -> str:
  """
  The version of the last release. This will be used as the version number if no
  Git information is available.
  """
  for version in all_versions():
    return version

  return "<unknown>"

def has_git() -> bool:
  """
  Return True if we are in a Git repository and have Git.
  """

  # return False if we don't have Git
  if shutil.which("git") is None:
    return False

  # return False if we have no Git repository information
  if not (Path(__file__).parent / "../../.git").exists():
    return False

  return True

def get_tag() -> Optional[str]:
  """
  Find the version tag of the current Git commit, e.g. v2020.05.03, if it
  exists.
  """
  try:
    tag = sp.check_output(["git", "describe", "--tags"], stderr=sp.DEVNULL)
  except sp.CalledProcessError:
    tag = None

  if tag is not None:
    tag = tag.decode("utf-8", "replace").strip()
    if re.match(r"v[\d\.]+$", tag) is None:
      # not a version tag
      tag = None

  return tag

def get_sha() -> str:
  """
  Find the hash of the current Git commit.
  """
  rev = sp.check_output(["git", "rev-parse", "--verify", "HEAD"])
  rev = rev.decode("utf-8", "replace").strip()

  return rev

def is_dirty() -> bool:
  """
  Determine whether the current working directory has uncommitted changes.
  """
  dirty = False

  p = sp.run(["git", "diff", "--exit-code"], stdout=sp.DEVNULL,
    stderr=sp.DEVNULL)
  dirty |= p.returncode != 0

  p = sp.run(["git", "diff", "--cached", "--exit-code"], stdout=sp.DEVNULL,
    stderr=sp.DEVNULL)
  dirty |= p.returncode != 0

  return dirty

def main(args: [str]) -> int:

  if len(args) != 2 or args[1] == "--help":
    sys.stderr.write(
      f"usage: {args[0]} file\n"
       " write version information as a C source file\n")
    return -1

  # get the contents of the old version file if it exists
  old = None
  if os.path.exists(args[1]):
    old = Path(args[1]).read_text()

  version = None

  # look for a version tag on the current commit
  if version is None and has_git():
    tag = get_tag()
    if tag is not None:
      version = f'{tag}{" (dirty)" if is_dirty() else ""}'

  # look for the commit hash as the version
  if version is None and has_git():
    rev = get_sha()
    assert rev is not None
    version = f'Git commit {rev}{" (dirty)" if is_dirty() else ""}'

  # fall back to our known release version
  if version is None:
    version = last_release()

  known_versions = ", ".join(f'"{v}"' for v in reversed(list(all_versions())))

  new = f"""\
#include <stddef.h>
#include <vimcat/version.h>

const char *vimcat_version(void) {{
  return "{version}";
}}

#ifdef __GNUC__
#define INTERNAL __attribute__((visibility("internal")))
#else
#define INTERNAL /* nothing */
#endif

const char *KNOWN_VERSIONS[] INTERNAL = {{
  {known_versions}
}};

size_t KNOWN_VERSIONS_LENGTH INTERNAL =
  sizeof(KNOWN_VERSIONS) / sizeof(KNOWN_VERSIONS[0]);
"""

  # If the version has changed, update the output. Otherwise we leave the old
  # contents – and more importantly, the timestamp – intact.
  if old != new:
    Path(args[1]).write_text(new)

  return 0

if __name__ == "__main__":
  sys.exit(main(sys.argv))
