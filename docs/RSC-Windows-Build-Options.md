# Robust Services Core: Windows Build Options

[_GlobalSettingsInclude.cmake_](/src/CMake/GlobalSettingsInclude.cmake)
specifies options that all projects share. For Windows, these are (with
the paths to them in VS2022's _Properties > Configuration Properties_)

- C/C++ > General > Common Language RunTime Support: No CLR Support

  The CLR and managed code are Windows-specific.
  
- C/C++ > General > Debug Information Format: /Z7

  This provides support for debugging, even in a release build.
  
- C/C++ > General > Warning Level: /Wall

  This enables all compiler warnings.  Some are explicitly disabled using
  the /wd option (see below).
  
- C/C++ > Optimization: /Od

  This disables optimizations, even in a release build, to facilitate
  debugging. However, a release build does use /GL (whole program
  optimization), /Ob1 (inlines), and /Oi (intrinsics). Overall, this
  results in a release build running about twice as slow as a fully
  optimized build, but over three times as fast as a debug build.
   
- C/C++ > Optimization > Omit Frame Pointers: Oy-

  This simplifies debugging.

- C/C++ > Preprocessor: OS_WIN

  This includes the Windows targets in *Sys\*.win.cpp* files.
  
- C/C++ > Code Generation > Enable C++ Exceptions: /EHa

  This is mandatory in order to catch Windows' structured exceptions.
  
- C/C++ > Code Generation > Basic Runtime Checks: /RTC1

  This detects things like the use of uninitialized variables and
out-of-bound array indices.
  
- C/C++ > All Options > Support Just My Code Debugging: /JMC-

  This supports debugging in Windows code.
  
- C/C++ > Command Line > Additional Options

  All compiler warnings except the following are enabled:
  - C4061: enumerator not handled by case label; default label exists
  - C4062: enumerator not handled by case label; no default label exists
  - C4100: unreferenced parameter
  - C4242: type conversion: possible loss of data
  - C4244: type conversion: possible loss of data
  - C4267: type conversion from size_t: possible loss of data
  - C4365: type conversion: signed/unsigned mismatch
  - C4514: unreferenced inline function removed
  - C4582: constructor is not implicitly called
  - C4623: default constructor implicitly defined as deleted
  - C4625: copy constructor implicitly defined as deleted
  - C4626: assignment operator implicitly defined as deleted
  - C4668: symbol not defined as a preprocessor macro [caused by Windows]
  - C4710: function not inlined
  - C4711: function inlined
  - C4715: not all paths return a value [_promoted to an error_]
  - C4820: padding added after member
  - C5026: move constructor implicitly defined as deleted
  - C5027: move assignment operator implicitly defined as deleted
  - C5045: will insert Spectre mitigation for memory load if /Qspectre
switch specified
  - C5219: type conversion: possible loss of data

  During code analysis, all warnings except the following are enabled.
  Microsoft's documentation for these can be found by searching on
  _Cnnnnn_:
  - C6389, C26135, C26403, C26409, C26429, C26430, C26432, C26436,
    C26439, C26440, C26446, C26447, C26451, C26455, C26458, C26462,
    C26466, C26475, C26481, C26482, C26485, C26486, C26487, C26488,
    C26489, C26490, C26492, C26493, C26494, C26496, C26497, C26812,
    C26818, C26822, C26823, C33010.

- Linker > Debugging > Generate Debug Info: /DEBUG:FULL
- Linker > Optimization > Enable COMDAT Folding: /OPT:NOICF

  These support debugging, even in a release build.
  
- Librarian > General > Additional Dependencies: Dbghelp.lib (for
*/nb*) and ws2_32.lib (for */nw*)
    
  These libraries contain *DbgHelp.h* and *Winsock2.h*, respectively.
