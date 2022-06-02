"""
Vimcat test suite
"""

import pytest
import subprocess
import tempfile
from pathlib import Path

@pytest.mark.parametrize("case", (
  "newline1.txt",
  "newline2.txt",
  "newline3.txt",
  "newline4.txt",
  "newline5.txt",
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
  if len(reference) > 0 and reference[-1] != "\n":
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

def test_tall():
  """
  check displaying a file that exceeds Vim’s 1000 line limit
  """

  with tempfile.TemporaryDirectory() as tmp:
    sample = Path(tmp) / "input.txt"

    # setup a file with many lines
    with open(sample, "wt") as f:
      for i in range(1024):
        f.write(f"line {i}\n")

    # ask `vimcat` to display it
    output = subprocess.check_output(["vimcat", "--debug", sample],
                                     universal_newlines=True)

  # confirm we got what we expected
  i = 0
  for line in output.splitlines():
    assert line == f"line {i}", f"incorrect output at line {i + 1}"
    i += 1
  assert i == 1024, "incorrect total number of lines"

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
