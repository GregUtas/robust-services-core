# Building RSC Using CMake

CMake generates the Visual Studio (VS) _vcxproj_ files that you would
otherwise have created using the Properties editor. Once that is done,
you can build in the usual way.

When using CMake, avoid changing the _vcxproj_ files that it generates,
either directly or through the Properties editor. The reason is that
CMake must _regenerate_ those files before you can build for another
target (Win32 or x64)--at which point your edits will be lost.
Instead of modifying the _vcxproj_ files, you must modify
[_GlobalSettingsInclude.cmake_](src/CMake/GlobalSettingsInclude.cmake)
or one of the _CMakeLists.txt_ files, so that CMake generates suitable
_vcxproj_ files. The CMake
[tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html#)
and
[reference](https://cmake.org/cmake/help/latest/index.html#) should help
if you need to do this. Many CMake questions are also addressed on
[stack**overflow**](https://stackoverflow.com/questions/tagged/cmake?sort=MostVotes&edited=true).

How VS has integrated support for CMake is described
[here](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-170).
I still haven't configured VS to seemlessly support CMake. However, the
workflow described below was adequate for building and running RSC.

## Generating _vcxproj_ files

- Open a Developer PowerShell in VS using _View > Terminal_.
- Navigate to the _src_ directory.
- Run CMake to generate the _vcxproj_ files for the desired target:

  - **cmake -G "Visual Studio 17 2022" -A Win32** _or_
  - **cmake -G "Visual Studio 17 2022" -A x64**

  The **-G** argument for earlier versions of VS can be found in the
"Visual Studio Generators" list provided
[here](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#id7).

Once you've run CMake, you'll be told that the project was modified, so go
ahead and reload it. You can then use the VS _Build_ menu in the usual way.

## Changing the target

A limitation of CMake is that it only generates _vcxproj_ files for one
target. Switching to another target means regenerating those files. If
you try to do this directly, by executing one of the above commands,
the following error appears:

`Either remove the CMakeCache.txt file and CMakeFiles directory or
choose a different binary directory.`

Go ahead and remove that file and directory from the _src_ directory.
You can also remove the two _ALL_BUILD_.* files, the two _ZERO_CHECK.*_
files, and the _cmake_install.cmake_ file, leaving only the _CMake_
directory and _CMakeList.txt_ file in the _src_ directory.

The following warning also appears, regardless of whether the command
is successfully executed:

`No source or binary directory provided.  Both will be assumed to be
the same as the current working directory, but note that this warning
will become a fatal error in future CMake releases.`


## Launching RSC

You can now launch RSC through the VS _Debug_ menu. If you get an error
saying that the _.exe_ couldn't be found, check that _rsc_ is set as
the startup project by right-clicking on it in the VS Solution Explorer
and selecting "Set as Startup Project".

## CMakeConverter

The tool [CMakeConverter](https://github.com/pavelliavonau/cmakeconverter)
significantly eased RSC's migration to CMake by generating the initial
_CMakeLists.txt_ files and a _GlobalSettingsInclude.cmake_ stub that could
be populated with RSC's compile and link options. If you want to migrate
to CMake but are new to it, you should definitely give this tool a try.

