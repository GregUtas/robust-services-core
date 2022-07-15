# Robust Services Core: Building with CMake

How VS has incorporated support for CMake is described
[here](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-170).
If you want to build RSC yourself, you need to install the components that
VS uses to support CMake.

## Launching Visual Studio

To use CMake in VS, you no longer open a solution. Instead, just launch
VS as an application. Then, under its **Get started** menu on the right,
select **Open a local folder**. In the File Explorer browser that pops up,
navigate to RSC's _src_ directory, click on it, and click the **Select
Folder** button. VS now notices the [_CMakeLists.txt_](/src/CMakeLists.txt)
file in that directory and starts to load all of the subprojects that it
requires. It therefore takes a little while before VS is ready to go.

## Building RSC

The file [_CMakeSettings.json_](/src/CMakeSettings.json) describes the
configurations that can be used to build RSC:

- **WSL-GCC-Debug**: Windows Subsystem for Linux using the gcc compiler
- **x64-Clang-Debug**: 64-bit Windows using the clang compiler
- **x64-Debug** and **x64-Release**: 64-bit Windows using the MSVC compiler
- **x86-Debug** and **x86-Release**: 32-bit Windows using the MSVC compiler

When you change the configuration, you may need to invoke _Project >
Delete Cache and Reconfigure_ before initiating a build. This erases the
old files that CMake generated and then generates those required for the
new configuration.

The x64 configurations are built using Ninja, which dramatically reduces
build times. I haven't succeeded in getting Ninja to support x86, so it
still builds the old, slow way.

If you notice CMake generating any _.vcxproj_ files, avoid modifying
them, either directly or through the Properties editor. The reason is
that CMake _regenerates_ those files before building for another
target (x86, x64, or Linux)--at which point your edits will be lost.

Instead of modifying _.vcxproj_ files, you need to modify
[_GlobalSettingsInclude.cmake_](/src/CMake/GlobalSettingsInclude.cmake),
or one of the _CMakeLists.txt_ files, to set the build parameters that
you require. The CMake
[tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html#)
and
[reference](https://cmake.org/cmake/help/latest/index.html#) should help
if you need to do this. Many CMake questions are also addressed on
[stack**overflow**](https://stackoverflow.com/questions/tagged/cmake?sort=MostVotes&edited=true).

## Launching RSC

You can now launch RSC through the VS _Debug_ menu. If _Debug > Start
Debugging_ is greyed out, use the _Select Startup Item..._ drop-down
menu, located to the right of the Configuration drop-down menu, to set
the _.exe_ that should be launched.

Make sure that RSC is launched from a directory below the [_src_](/src)
directory, as described in the [installation guide](/docs/Installing.md).

## CMakeConverter

The tool [CMakeConverter](https://github.com/pavelliavonau/cmakeconverter)
significantly eased RSC's migration to CMake by analyzing _.vcxproj_ files
to generate the initial _CMakeLists.txt_ files and a
_GlobalSettingsInclude.cmake_ stub that could be populated with RSC's
compile and link options. If you have a VS solution that you want to
migrate to CMake but are new to it, give this tool a try.
