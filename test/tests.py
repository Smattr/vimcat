"""
Vimcat test suite
"""

import pytest
import subprocess
from pathlib import Path

@pytest.mark.parametrize("case", ("utf-8.txt", "utf-8_1.txt", "utf-8_2.txt",
                                  "utf-8_3.txt", "utf-8_4.txt", "utf-8_5.txt",
                                  "utf-8_6.txt"))
@pytest.mark.xfail(strict=True)
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
