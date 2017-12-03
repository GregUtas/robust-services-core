
# Executables

This directory contains RSC executables.
* `rscwin32.exe`: debug build for 32-bit Windows (Windows 10)
* `rscwin64.exe`: debug build for 64-bit Windows (Windows 10)

Save an executable in a directory that allows it to locate the
[configuration file](/input/element.config.txt).  When the program
starts, it notes the path from which it was launched and searches
for the string `rsc/rsc/` on that path.  The configuration file must
then appear as `.../rsc/rsc/input/element.config.txt`.  If it is not
found, a warning appears on the console and default values are used
for all configuration parameters.
