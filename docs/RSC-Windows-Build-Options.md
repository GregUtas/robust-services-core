# Robust Services Core: Windows Build Options

In Properties > Configuration Properties, the following options are set
for all projects:

- CC/C++ > General > Common Language RunTime Support: No CLR Support

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
  - C5045: will insert Spectre mitigation for memory load if /Qspectre switch specified
  - C5219: type conversion: possible loss of data
  - C26812: prefer 'enum class' over 'enum'
  - C33010: unchecked lower bound for enum used as index

- Linker > General > Enable Incremental Linking: /INCREMENTAL:NO
- Linker > Debugging > Generate Debug Info: /DEBUG:FULL
- Linker > Optimization > References: /OPT:NOREF
- Linker > Optimization > Enable COMDAT Folding: /OPT:NOICF

  These support debugging, even in a release build.
  
- Librarian > General > Additional Dependencies: Dbghelp.lib (for */nb*) and ws2_32.lib (for */nw*)
    
  These libraries contain *DbgHelp.h* and *Winsock2.h*, respectively.
