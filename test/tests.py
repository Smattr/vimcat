"""
Vimcat test suite
"""

import pytest
import subprocess
from pathlib import Path

@pytest.mark.xfail(strict=True)
def test_utf8():
  """
  check `vimcat` can deal with UTF-8 characters of any length
  """

  # run `vimcat` on a sample containing characters of various lengths
  input = Path(__file__).parent / "utf-8.txt"
  assert input.exists(), "missing test case input"
  output = subprocess.check_output(["vimcat", input], universal_newlines=True)

  # read the sample with Python, which we know understands UTF-8 correctly
  with open(input, "rt", encoding="utf-8") as f:
    reference = f.read().strip()

  # `vimcat` should have given us the same, under the assumption no syntax
  # highlighting of basic text files is enabled
  assert output.strip() == reference, "incorrect UTF-8 decoding"