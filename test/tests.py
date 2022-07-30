"""
Vimcat test suite
"""

import os
import pytest
import subprocess
import tempfile
from pathlib import Path
from typing import Optional

@pytest.mark.parametrize("colour", (None, "always", "auto", "never"))
@pytest.mark.parametrize("no_color", (False, True))
@pytest.mark.parametrize("t_Co", (2, 8, 16, 88, 256, 16777216))
@pytest.mark.parametrize("termguicolors", (False, True))
def test_colour(colour: Optional[str], no_color: bool, t_Co: int,
                termguicolors: bool):
  """
  `vimcat` should obey the user’s colour preferences
  """

  env = os.environ.copy()
  if no_color:
    env["NO_COLOR"] = "1"
  elif "NO_COLOR" in env:
    del env["NO_COLOR"]

  with tempfile.TemporaryDirectory() as tmp:

    # write a vimrc to force syntax highlighting and 8-bit colour
    with open(Path(tmp) / ".vimrc", "wt") as f:
      f.write(f"syntax on\nset t_Co={t_Co}\n")
      if termguicolors:
        f.write("set termguicolors\n")
    env["HOME"] = tmp

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
def test_combining_characters():
  """
  UTF-8 combining characters should be rendered in the correct terminal cell
  """

  # We cannot directly read the virtual terminal interface through `vimcat` and
  # we cannot distinguish which column a given character we receive came from.
  # So we use a trick where we stick the combining character just across the
  # border of the maximum column Vim will render to. So if we get the combining
  # right, this should be visible, and if not it will be invisible.

  with tempfile.TemporaryDirectory() as tmp:
    sample = Path(tmp) / "input.txt"

    # write a file containing a trailing combining character
    with open(sample, "wb") as f:
      for _ in range(VIM_COLUMN_LIMIT - 1):
        f.write(b" ")
      f.write(b"e")
      f.write(b"\xcc\x81")

    # ask `vimcat` to render it
    output = subprocess.check_output(["vimcat", "--debug", sample])

  prefix = b" " * (VIM_COLUMN_LIMIT - 1)
  assert output.startswith(prefix), "incorrect leading space"

  assert output[len(prefix):] == b"e\xcc\x81\n", "truncated combining character"

@pytest.mark.parametrize("case", (
  "newline1.txt",
  "newline2.txt",
  "newline3.txt",
  "newline4.txt",
  "newline5.txt",
  "newline6.txt",
  "newline7.txt",
))
def test_newline(case: str):
  """
  check `vimcat` deals with various newline ending/not-ending correctly
  """

  # run `vimcat` on some sample input
  input = Path(__file__).parent / case
  assert input.exists(), "missing test case input"
  output = subprocess.check_output(["vimcat", "--debug", input],
                                   universal_newlines=True)

  # read the sample in Python
  reference = input.read_text()

  # if it was non-empty and did not end in a newline, `vimcat` should have added
  # one
  if len(reference) == 0 or reference[-1] != "\n":
    reference += "\n"

  assert output == reference, "incorrect newline handling"

def test_no_file():
  """
  passing a non-existent file should produce no output and an error message
  """

  with tempfile.TemporaryDirectory() as tmp:
    input = Path(tmp) / "no-file.txt"

    p = subprocess.run(["vimcat", input], capture_output=True)

  assert p.returncode != 0, "EXIT_SUCCESS status with non-existent file"
  assert p.stdout == b"", "output for non-existent file"
  assert p.stderr != b"", "no error message for non-existent file"

VIM_LINE_LIMIT = 1000
"""
maximum number of terminal lines Vim will render
"""

@pytest.mark.parametrize("height",
  list(range(VIM_LINE_LIMIT - 2, VIM_LINE_LIMIT + 3)) +
  list(range(2 * VIM_LINE_LIMIT - 2, 2 * VIM_LINE_LIMIT + 3))
)
def test_tall(height: int):
  """
  check displaying a file near the boundaries of Vim’s line limit
  """

  with tempfile.TemporaryDirectory() as tmp:
    sample = Path(tmp) / "input.txt"

    # setup a file with many lines
    with open(sample, "wt") as f:
      for i in range(height):
        f.write(f"line {i}\n")

    # ask `vimcat` to display it
    output = subprocess.check_output(["vimcat", "--debug", sample],
                                     universal_newlines=True)

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
def test_utf8(case: str):
  """
  check `vimcat` can deal with UTF-8 characters of any length
  """

  # run `vimcat` on a sample containing characters of various lengths
  input = Path(__file__).parent / case
  assert input.exists(), "missing test case input"
  output = subprocess.check_output(["vimcat", "--debug", input],
                                   universal_newlines=True)

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
def test_wide(width: int):
  """
  at least as many columns as the Vim render limit should be displayed
  """

  with tempfile.TemporaryDirectory() as tmp:
    sample = Path(tmp) / "input.txt"

    # setup a file with a wide line:
    with open(sample, "wt") as f:
      for _ in range(width):
        f.write("a")
      f.write("\n")

    # ask `vimcat` to display it
    output = subprocess.check_output(["vimcat", "--debug", sample],
                                     universal_newlines=True)

  # confirm we got at least as many columns as expected
  if width <= VIM_COLUMN_LIMIT:
    reference = "a" * width + "\n"
    assert output == reference, "incorrect wide line rendering"
  else:
    reference = "a" * VIM_COLUMN_LIMIT
    assert output.startswith(reference), "incorrect wide line rendering"
