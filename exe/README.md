# Executables

This directory used to contain RSC executables.  They're still
available, but commits are now being made to the master branch,
with stable points tagged as releases. If you go to RSC's
[tags](https://github.com/GregUtas/robust-services-core/tags) page,
you will see all the release tags. The download for each release
includes the following:
* _rscwin32d.exe_: debug build for 32-bit Windows (Windows 10)
* _rscwin64d.exe_: debug build for 64-bit Windows (Windows 10)
* _rscwin32r.exe_: release build for 32-bit Windows (Windows 10)
* _rscwin64r.exe_: release build for 64-bit Windows (Windows 10)

These executables are provided so that you don't have to build RSC
yourself. They are _not_ installers, and you must still have the full
repository installed, along with the VS2017 (or similar) environment
that an _.exe_ requires.

Save an executable in a directory--this one is a good choice--that
allows it to locate the [configuration file](/input/element.config.txt).
When the program starts, it notes the path from which it was launched and
searches for _rsc/_ upwards on that path.  The configuration file must
then appear as _.../rsc/input/element.config.txt_.  If it isn't found,
a warning appears on the console and default values are used for all
configuration parameters.

Note that a release build disables a number of optimizations so that
it can actually be debugged. It runs about 3.5 times faster than a
debug build, but only about half as fast as a release build that is
fully optimized.
