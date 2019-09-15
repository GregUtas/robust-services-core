# Executables

This directory contains RSC executables.
* _rscwin32.exe_: debug build for 32-bit Windows (Windows 10)
* _rscwin64.exe_: debug build for 64-bit Windows (Windows 10)

Save an executable in a directory that allows it to locate the
[configuration file](/input/element.config.txt).  When the program
starts, it notes the path from which it was launched and searches
for _rsc/_ upwards on that path.  The configuration file must
then appear as _.../rsc/input/element.config.txt_.  If it is not
found, a warning appears on the console and default values are used
for all configuration parameters.
