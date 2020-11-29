# Robust Services Core: Windows Build Options

In Properties > Configuration Properties, the following options are set
for all projects:

- C/C++ > General > Common Language RunTime Support: No CLR Support

  The CLR and managed code are Windows-specific.
  
- C/C++ > General > Debug Information Format: /Z7

  This provides full symbol information, even in a release build.
  
- C/C++ > General > Warning Level: /W4

  This enables all compiler warnings.  A few are explicitly disabled using
  the /wd option (see below).
  
- C/C++ > Optimization: /Od

  This disables optimizations, even in a release build, to facilitate
  debugging. Taken as a whole, the options for a release build cause it
  to run about twice as slow as a fully optimized build, but over three
  times as fast as a debug build.
  
- C/C++ > Optimization > Omit Frame Pointers: Oy-

  Keeping frame pointers ensures that `SysThreadStack` will work.

- C/C++ > Preprocessor: OS_WIN

  This includes the Windows targets in *Sys\*.win.cpp* files.
  
- C/C++ > Code Generation > Enable C++ Exceptions: /EHa

  This is mandatory in order to catch Windows' structured exceptions.
  
- C/C++ > Code Generation > Basic Runtime Checks: /RTC1 (debug builds)

  This detects things like the use of uninitialized variables and out-of-bound array indices.
  
- C/C++ > Browse Information > Enable Browse Information: /FR

  Might as well enable browsing.
  
- C/C++ > Command Line > Additional Options: /wd4100 /wd4127 /wd4244 /wd4481 /we4715
    
  The first four are innocuous.  4715 is treated as an error.

- Linker > General > Enable Incremental Linking: /INCREMENTAL:NO
- Linker > Debugging > Generate Debug Info: /DEBUG:FULL
- Linker > Optimization > References: /OPT:NOREF
- Linker > Optimization > Enable COMDAT Folding: /OPT:NOICF

  These support debugging, even in a release build.
  
- Librarian > General > Additional Dependencies: Dbghelp.lib (for */nb*) and ws2_32.lib (for */nw*)
    
  These libraries contain *DbgHelp.h* and *Winsock2.h*, respectively.
