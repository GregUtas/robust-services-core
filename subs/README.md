# Substitute header files

This directory contains C++ header files that declare what RSC uses
from external libraries, whether standard C++ libraries or those of
the target platform (currently Windows).

These headers are only used during `CodeTools` compiles (the `>parse`
command).  They have been
simplified to remove things that RSC does not require.  In other cases,
what appears in external headers has been modified to avoid having to
enhance the [parser](/ct/Parser.h).  For example, some C-style constructs
(e.g. `#define` to define a constant, or `typedef struct`) have been
converted to C++ analogs.  In other cases, STL templates have been
simplified.
