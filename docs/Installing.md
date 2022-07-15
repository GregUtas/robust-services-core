# Robust Services Core: Installation Guide

To function correctly, RSC must be able to find its [_input_](/input) and
[_help_](/help) directories when it is launched. To ensure that this occurs,
launch RSC from a directory below the [_src_](/src) directory:

- If you downloaded an executable provided with a release, move it to the
  [_exe_](/src/exe) directory before launching it.

- If you built RSC yourself, your executable probably ended up in a directory
  that was created below the _src_ directory.  If it didn't, move it
  to the _exe_ directory mentioned above before launching it.

When you launch RSC, it searches upward on the path to its executable to
find \<_dir_>, the directory immediately above _src_. Its configuration
file must then appear as _.../\<dir>/input/element.config.txt_. If it can't
find this file, a warning appears on the console and default values are used
for all configuration parameters. Similarly, the _help_ directory must appear
as _.../\<dir>/help_.
