"""
Vimcat test suite
"""

import pytest
import subprocess
from pathlib import Path

@pytest.mark.parametrize("case", (
  "newline1.txt",
  "newline2.txt",
  "newline3.txt",
  "newline4.txt",
  "newline5.txt",
))
@pytest.mark.xfail(strict=True)
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
  with open(input, "rt", encoding="utf-8") as f:
    reference = f.read().strip()

  # `vimcat` should have given us the same, under the assumption no syntax
  # highlighting of basic text files is enabled
  assert output.strip() == reference, "incorrect UTF-8 decoding"
