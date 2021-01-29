# Executables

This directory contains RSC executables.
* _rscwin32d.exe_: debug build for 32-bit Windows (Windows 10)
* _rscwin64d.exe_: debug build for 64-bit Windows (Windows 10)
* _rscwin32r.exe_: release build for 32-bit Windows (Windows 10)
* _rscwin64r.exe_: release build for 64-bit Windows (Windows 10)

Save an executable in a directory that allows it to locate the
[configuration file](/input/element.config.txt).  When the program
starts, it notes the path from which it was launched and searches
for _rsc/_ upwards on that path.  The configuration file must
then appear as _.../rsc/input/element.config.txt_.  If it is not
found, a warning appears on the console and default values are used
for all configuration parameters.

Note that a release build disables a number of optimizations so that
it can actually be debugged. Nevertheless, it runs about 3.5 times
faster than a debug build, but about half as fast as a release
build that is fully optimized.
