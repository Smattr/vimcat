Change log
==========

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
