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

After downloading an executable, move it to the [_exe_](/src/exe)
directory before launching it, as described in the
[installation guide](/docs/Installing.md).

A release build disables a number of optimizations so that it can
actually be debugged. It runs about 3&#189; times as fast as a debug
build, but only about half as fast as a fully optimized release build.
