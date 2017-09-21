# robust-services-core

This repository contains
1. A framework for developing robust applications in C++.
2. An application built using the framework.
3. Tools for the static analysis of C++ software.

## License
The software in this repository was developed by Greg Utas, who currently
retains copyright.

## Robust Services Core
The framework that supports robust applications is referred to as the
*Robust Services Core* (RSC).  Many of the design patterns in RSC are used in
robust telecom products, where the term *carrier-grade* refers to a server with
at least five-nines (99.999%) availability.  A pattern language summarizing
these patterns appears in the [second chapter](/docs/RCS-chapter-2.pdf) of
*Robust Communications Software*.  The document
[*RSC Product Overview*](/docs/RSC-Product-Overview.pdf) discusses which
patterns are currently available in this repository and the primary code
files that implement them.

## POTS application
Including an application with a framework serves to test it and illustrate its
use.  This repository therefore includes a POTS (Plain Ordinary Telephone
Service) application.  POTS was chosen for several reasons.  For one thing,
the author had extensive experience with such applications while working
as a software architect in the telecom industry.  But more importantly, POTS
is a non-trivial application, and everyone has a reasonable understanding of what
it does.  You should therefore be able to figure out what the POTS code is doing
without having read a large specification.  An overview of the POTS application
is provided [here](/docs/RSC-POTS-Application.md).

## C++ static analysis tool
The development of RSC got sidetracked when the author decided to develop C++
static analysis tools.  This toolset detects violations of various C++ design
guidelines, such as those found in Scott Myer's *Effective C++*.  It also analyzes
`#include` lists to find `#include`'s that should be added or deleted.  An overview
of the toolset is provided [here](docs/RSC-Cxx-Static-Analysis-Tools.md).

## Building an executable
RSC requires C++11.

RSC is currently implemented on Windows, where it runs as a console application.
However, it defines an abstraction layer, in the form of generic C++ `.h`'s and
platform-specific `.cpp`'s, that should allow it to be ported to other systems
fairly easily.  Two executables, for 32-bit and 64-bit Windows, are provided
[here](/executables).

The directories that contain source code, and their dependencies, are listed in
the comments that precede the implementation of [`main`](/rsc/main.cpp).  Each
of these directories is built as a separate static library, with `main` residing
in its own directory.

RSC was developed using Visual Studio 2017.  If this is also your development
environment, the `.vcxproj` (project) files in this repository should already
provide most of the build instructions that you need.  However, you will need
to change the paths to where the source code is located.  It's probably
easiest to do this by opening the `.vcxproj` files in Notepad and replacing
occurrences of `C:\Users\gregu\Documents\rsc\rsc` (the directory that contains the
source code on my PC) with the top-level directory into which you downloaded the
repository.

## Running the executable

Before you run the executable, you also need to change each path (as described in
the above paragraph) in the [configuration file](input/element.config.txt), which
is read when the program is initializing during startup.

During initialization, the program displays each module as it is initialized.  (A
*module* is currently equivalent to a static library.)  After all modules have
initialized, the CLI prompt `nb>` appears to indicate that CLI commands in the
`nb` directory are available.  The information written to the console during
startup is shown [here](/docs/output/startup.txt), and a list of all CLI commands
is provided [here](/docs/output/help.cli.txt).

If you enter `read saveinit` as the first CLI command, a function trace of the
initialization, which starts even before the invocation of `main()`, is generated.
This trace should look a lot like [this](/docs/output/init.trace.txt).  Each function
that appears in such a trace invoked `Debug::ft`, which records the following:
  * the function's name
  * the time when it was invoked
  * the thread that invoked it
  * its depth (in frames) on the stack: this controls indentation so that you tell how
function calls were nested
  * the total time spent in the function (in microseconds)
  * the net time spent in the function (in microseconds)

All output appears in the directory specified by `OutputPath` in the configuration file.
In addition to any specific output that you request, such as the initialization trace,
every CLI session produces
  * a `console` file (a transcript of the CLI commands that you entered and what was
written to the console)
  * a `log` file (events that were written to the console asynchronously)
  * a `stats` file (generated periodically to report system statistics)

The numeric string *`yymmdd-hhmmss`* is appended to the names of these files to record
the time when the system initialized (for a `console` or `log` file) or when the report
was generated (for a `stats` file).

## Testing

Most of the files in the [inputs](/inputs) directory are test scripts.  The document that
describes the [POTS application](/docs/RSC-Pots-Application.md) also discusses its tests,
which exercise a considerable portion of the RSC software.  The tests described here are
therefore rather tactical and minimal.

  1. Twenty scripts test the *Safety Net* aspect of the `Thread` class.  Most of these cause
a POSIX signal to be raised.  POSIX signals are handled by throwing a C++ exception that is
caught in `Thread.Start`, after which the appropriate recovery action is taken.  Getting the
safety net to work is likely to be one of the challenges when porting RSC to another platform,
which is why these tests are provided.  All of the safety net scripts can be executed with the
command `>read test.trap.critical`.  For each test, the following are generated:
    * A function trace (`*.trace.txt`).
    * A function profile (`*.funcs.txt`), which lists the functions invoked and the total net
time spent in each.  This report is not particularly useful here, but is helpful when trying
to determine which functions to focus on when trying to improve real-time performance.
    * A scheduler trace (`*.sched.txt`).  **CONTINUE HERE**.
    * A console file of the test (`*.cli.txt).
  2. Entering `>nt` in the CLI enters the "nt" *increment* (a set of CLI commands).  This
increment provides a number of commands for testing the queue and time functions found in
`Q1Way.h`, `Q2Way.h`, and `SysTime.h`.

