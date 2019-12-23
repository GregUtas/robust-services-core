# Robust Services Core: Windows Build Options

In Properties > Configuration Properties, the following options are set
for all projects:

- C/C++ > General > Common Language RunTime Support: no CLR support

  The CLR and managed code are Windows specific and degrade performance.
  
- C/C++ > General > Debug Information Format: /Z7

  This provides full symbol information, even in a release build.
  
- C/C++ > General > Warning Level: /W4

  This enables all compiler warnings.  A few are explicitly disabled using
  the /wd option (see below).
  
- C/C++ > Optimization > Inline Function Expansion: /Ob1 (release builds)

  This limits inlining to functions that are tagged as inline or that are
  implemented in the class definition.  Aggressive inlining makes debugging
  more difficult.
  
- C/C++ > Optimization > Omit Frame Pointers: Oy-

  Keeping frame pointers ensures that `SysThreadStack` will work.
  
- C/C++ > Code Generation > Enable C++ Exceptions: /EHa

  This is mandatory in order to catch Windows' structured exceptions.
  
- C/C++ > Code Generation > Basic Runtime Checks: /RTC1 (debug builds)

  This detects things like the use of uninitialized variables and out-of-bound array indices.
  
- C/C++ > Code Generation > Struct Member Alignment: /Z4 or /Z8

  If the machine is 32 (64) bits, then align to 32 (64) bits.
  
- C/C++ > Browse Information > Enable Browse Information: /FR

  Might as well enable browsing.
  
- C/C++ > Command Line > Additional Options: /wd4100 /wd4127 /wd4244 /wd4481 /we4715
    
  The first four are innocuous.  4715 is treated as an error.
  
- Librarian > General > Additional Dependencies: Dbghelp.lib (for */nb*) and ws2_32.lib (for */nw*)
    
  These libraries contain *DbgHelp.h* and *Winsock2.h*, respectively.
