Change log
==========

v2022.09.19
-----------
* The ``vimcat_read`` callback now accepts a non-const ``line``
  parameter. This allows callers to modify the pointed to data, should they find
  it convenient (commit 7a4ded792e0ef47ba356dc504dff8dd4f8fff64e).
* A new API function ``vimcat_have_vim`` has been added to check for Vim
  availability (commit bfae8955b2445ae6d3c7e58d5cd75a7684bd091f).
* ``vimcat`` now checks for Vim availability before attempting highlighting
  (commit 58ed56403e204ed9824c22c84e8b0a678bf1f332).
* When running out of memory, ``vimcat --help`` will now more reliably report
  this (commit 36ba8c93f76f6fe7604f70ee611dbae555fea351).

v2022.07.30
-----------

* Bug fix: handling of newline characters has been corrected (commit
  3eaffb0ed3e6612ace7a7f1be6f6e5899cb194d8).
* A first-run check has been introduced to warn users of risks (commit
  e4517040c357c5289ad2dd00325b65fdd54d431c).
* 24-bit colour is supported (commit f0418618dfd62d70123f7503338c973fccb37e82).
* ``set title`` in your .vimrc no longer causes failures (commit
  7cb994645581eac8346b135fc2614cc6a3f54221).
* ``--debug`` prints more information (commits
  9447055479b1850a1ee9eea9801d404cceece39f,
  71bfcd997e84012d30ea0a7172118e83fcec975e).
* libvimcat: an API function for reading a single line was added (commit
  9e311d23c3d27cc4b64ef73471fbab1a37249f8a).

v2022.06.23
-----------

* libvimcat: a symbol has been renamed that may have caused conflicts when
  static linking (commit ef594bb764e8a316f3dd4f158c9aa109852037e6).

v2022.06.20
-----------

* A ``--colour`` command line option is now supported for controlling whether
  output is coloured or not (commit ee9da8665d537e77ed45115b028b8cd12b3894c7).
* Colour depths beyond 8-colour (2-colour, 88-colour, 8-bit colour, 24-bit
  colour) are now correctly supported (commits
  6c2d2227a9d4597eea115b8a00c61d118573bfb6,
  c4292ae8de95d7f532a40a357fcabe7ea321d4ce,
  ccd358b837c4259d07a6c35a2d3e04d4b5ae1c8d,
  a77e7d676714cb88d24af7d9c77edbd6787f0e7d,
  8e59885b4d5009794c72e8dda81c00f8def2c691).
* libvimcat: functions for comparing one library version to another have been
  added (commit 2a5fb4a52fb07c36e97cb64b69527b8404a11668).

v2022.06.11
-----------

* Initial release.
