# Robust Services Core: Executables

RSC commits are made to the main branch, with stable points tagged
as releases. If you go to the
[tags](https://github.com/GregUtas/robust-services-core/tags) page,
you will see all the release tags. The Assets that can be downloaded
with a release include
* _rscwin64d.exe_: clang debug build for 64-bit Windows (Windows 11)
* _rscwin64r.exe_: MSVC release build for 64-bit Windows (Windows 11)
* _rsclin64d_: gcc debug build for 64-bit Linux (WSL2 on Ubuntu)

These executables are provided so that you don't have to build RSC
yourself. They are _not_ installers, so you must still have the full
repository installed, along with the Visual Studio components that an
_.exe_ requires (VS2022, although earlier versions may also work).

Download an executable to the [_src_](/src) directory or one below
it. This will allow the executable to locate its
[configuration file](/input/element.config.txt). When RSC starts, it
notes the path from which it was launched and searches for _rsc/_
upwards on that path. The configuration file must then appear as
_.../rsc/input/element.config.txt_. If it isn't found, a warning
appears on the console and default values are used for all
configuration parameters.

A release build disables a number of optimizations so that it can
actually be debugged. It runs about 3&#189; times as fast as a debug
build, but only about half as fast as a fully optimized release build.
