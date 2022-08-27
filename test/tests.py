"""
Vimcat test suite
"""

import os
import pytest
import re
import shutil
import subprocess
from pathlib import Path
from typing import Dict, Optional

def make_vimcatrc(home: Path):
  """
  create an empty ~/.vimcatrc to pass the consent check
  """
  (home / ".vimcatrc").write_text("")

def set_home(home: Path) -> Dict[str, str]:
  """
  setup an environment using the given path as ${HOME}
  """
  env = os.environ.copy()
  env["HOME"] = str(home)
  make_vimcatrc(home)
  return env

@pytest.mark.parametrize("colour", (None, "always", "auto", "never"))
@pytest.mark.parametrize("no_color", (False, True))
@pytest.mark.parametrize("t_Co", (2, 8, 16, 88, 256, 16777216))
@pytest.mark.parametrize("termguicolors", (False, True))
@pytest.mark.parametrize("title", (False, True))
def test_colour(tmp_path: Path, colour: Optional[str], no_color: bool,
                t_Co: int, termguicolors: bool, title: bool):
  """
  `vimcat` should obey the user’s colour preferences
  """

  env = set_home(tmp_path)
  if no_color:
    env["NO_COLOR"] = "1"
  elif "NO_COLOR" in env:
    del env["NO_COLOR"]

  # write a vimrc to force syntax highlighting and 8-bit colour
  with open(tmp_path / ".vimrc", "wt") as f:
    f.write(f"syntax on\nset t_Co={t_Co}\n")
    if termguicolors:
      f.write("set termguicolors\n")
    if title:
      f.write("set title\n")

  args = ["vimcat", "--debug"]
  if colour is not None:
    args += [f"--colour={colour}"]

  # highlight a C file
  source = Path(__file__).parent / "test_version_le.c"
  output = subprocess.check_output(args + ["--", source], env=env)

  # was there a Control Sequence Identifier in the output?
  contains_csi = b"\033[" in output

  # allow no colour in monochrome mode, as it may be unused/unsupported
  if t_Co == 2 and not contains_csi:
    return

  if colour == "auto" or colour is None:
    assert contains_csi != no_color, "incorrect NO_COLOR handling"
  elif colour == "always":
    assert contains_csi, "incorrect --colour=always behaviour"
  else:
    assert colour == "never"
    assert not contains_csi, "incorrect --colour=never behaviour"

VIM_COLUMN_LIMIT = 10000
"""
maximum number of terminal columns Vim will render
"""

@pytest.mark.xfail(strict=True)
def test_combining_characters(tmp_path: Path):
  """
  UTF-8 combining characters should be rendered in the correct terminal cell
  """

  # We cannot directly read the virtual terminal interface through `vimcat` and
  # we cannot distinguish which column a given character we receive came from.
  # So we use a trick where we stick the combining character just across the
  # border of the maximum column Vim will render to. So if we get the combining
  # right, this should be visible, and if not it will be invisible.

  sample = tmp_path / "input.txt"

  # write a file containing a trailing combining character
  with open(sample, "wb") as f:
    for _ in range(VIM_COLUMN_LIMIT - 1):
      f.write(b" ")
    f.write(b"e")
    f.write(b"\xcc\x81")

  env = set_home(tmp_path)

  # ask `vimcat` to render it
  output = subprocess.check_output(["vimcat", "--debug", sample], env=env)

  prefix = b" " * (VIM_COLUMN_LIMIT - 1)
  assert output.startswith(prefix), "incorrect leading space"

  assert output[len(prefix):] == b"e\xcc\x81\n", "truncated combining character"

@pytest.mark.parametrize("debug", (False, True))
def test_consent(tmp_path: Path, debug: bool):
  """
  Vimcat should refuse to run without ~/.vimcatrc
  """

  # like `set_home` but exclude creating a ~/.vimcatrc
  env = os.environ.copy()
  env["HOME"] = str(tmp_path)

  # pick an arbitrary file to cat
  subject = Path(__file__).resolve()

  # run vimcat
  args = ["vimcat"]
  if debug:
    args += ["--debug"]
  args += [subject]
  ret = subprocess.call(args, env=env)

  assert ret != 0, "vimcat ran successfully without ~/.vimcatrc"

@pytest.mark.parametrize("case", (
  "newline1.txt",
  "newline2.txt",
  "newline3.txt",
  "newline4.txt",
  "newline5.txt",
  "newline6.txt",
  "newline7.txt",
))
def test_newline(tmp_path: Path, case: str):
  """
  check `vimcat` deals with various newline ending/not-ending correctly
  """

  env = set_home(tmp_path)

  # run `vimcat` on some sample input
  input = Path(__file__).parent / case
  assert input.exists(), "missing test case input"
  output = subprocess.check_output(["vimcat", "--debug", input],
                                   universal_newlines=True, env=env)

  # read the sample in Python
  reference = input.read_text()

  # if it was non-empty and did not end in a newline, `vimcat` should have added
  # one
  if len(reference) == 0 or reference[-1] != "\n":
    reference += "\n"

  assert output == reference, "incorrect newline handling"

def test_no_file(tmp_path: Path):
  """
  passing a non-existent file should produce no output and an error message
  """

  input = tmp_path / "no-file.txt"
  env = set_home(tmp_path)

  p = subprocess.run(["vimcat", input], capture_output=True, env=env)

  assert p.returncode != 0, "EXIT_SUCCESS status with non-existent file"
  assert p.stdout == b"", "output for non-existent file"
  assert p.stderr != b"", "no error message for non-existent file"

def test_no_vim(tmp_path: Path):
  """
  if `vim` is not installed, we should get a reasonable error message
  """
  env = set_home(tmp_path)

  # construct an absolute path to vimcat
  vimcat = shutil.which("vimcat")
  assert vimcat is not None, "vimcat not found"
  vimcat = Path(vimcat).resolve()

  # blank `$PATH` so `vim` cannot be found
  env["PATH"] = ""

  # run `vimcat` on an arbitrary file
  src = Path(__file__).resolve()
  p = subprocess.run([vimcat, src], capture_output=True,
                     universal_newlines=True, env=env)

  assert p.returncode != 0, \
    "vimcat exited with success even without vim available"
  assert re.search(r"\bvim\b", p.stderr) is not None, \
    "error message did not mention vim"

VIM_LINE_LIMIT = 1000
"""
maximum number of terminal lines Vim will render
"""

@pytest.mark.parametrize("height",
  list(range(VIM_LINE_LIMIT - 2, VIM_LINE_LIMIT + 3)) +
  list(range(2 * VIM_LINE_LIMIT - 2, 2 * VIM_LINE_LIMIT + 3))
)
def test_tall(tmp_path: Path, height: int):
  """
  check displaying a file near the boundaries of Vim’s line limit
  """

  sample = tmp_path / "input.txt"
  env = set_home(tmp_path)

  # setup a file with many lines
  with open(sample, "wt") as f:
    for i in range(height):
      f.write(f"line {i}\n")

  # ask `vimcat` to display it
  output = subprocess.check_output(["vimcat", "--debug", sample],
                                   universal_newlines=True, env=env)

  # confirm we got what we expected
  i = 0
  for line in output.splitlines():
    assert line == f"line {i}", f"incorrect output at line {i + 1}"
    i += 1
  assert i == height, "incorrect total number of lines"

@pytest.mark.parametrize("case", (
  "utf-8.txt",
  "utf-8_1.txt",
  "utf-8_2.txt",
  "utf-8_3.txt",
  "utf-8_4.txt",
  "utf-8_5.txt",
  "utf-8_6.txt",
))
def test_utf8(tmp_path: Path, case: str):
  """
  check `vimcat` can deal with UTF-8 characters of any length
  """

  env = set_home(tmp_path)

  # run `vimcat` on a sample containing characters of various lengths
  input = Path(__file__).parent / case
  assert input.exists(), "missing test case input"
  output = subprocess.check_output(["vimcat", "--debug", input],
                                   universal_newlines=True, env=env)

  # read the sample with Python, which we know understands UTF-8 correctly
  reference = input.read_text(encoding="utf-8").strip()

  # `vimcat` should have given us the same, under the assumption no syntax
  # highlighting of basic text files is enabled
  assert output.strip() == reference, "incorrect UTF-8 decoding"

def test_version_le():
  """
  version comparison API should behave as expected
  """
  subprocess.check_call(["test_version_le"])

@pytest.mark.parametrize("width",
  list(range(VIM_COLUMN_LIMIT - 2, VIM_COLUMN_LIMIT + 3)))
def test_wide(tmp_path: Path, width: int):
  """
  at least as many columns as the Vim render limit should be displayed
  """

  sample = tmp_path / "input.txt"
  env = set_home(tmp_path)

  # setup a file with a wide line:
  with open(sample, "wt") as f:
    for _ in range(width):
      f.write("a")
    f.write("\n")

  # ask `vimcat` to display it
  output = subprocess.check_output(["vimcat", "--debug", sample],
                                   universal_newlines=True, env=env)

  # confirm we got at least as many columns as expected
  if width <= VIM_COLUMN_LIMIT:
    reference = "a" * width + "\n"
    assert output == reference, "incorrect wide line rendering"
  else:
    reference = "a" * VIM_COLUMN_LIMIT
    assert output.startswith(reference), "incorrect wide line rendering"
