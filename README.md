# robust-services-core

This repository contains
1. A framework for developing robust applications in C++.
2. An application built using the framework.
3. Tools for the static analysis of C++ software.

## Robust Services Core

The framework that supports robust applications is referred to as the *Robust Services
Core* (RSC).  RSC will put your project on the right path and jump-start it if you're
developing or reengineering a system whose requirements can be characterized as

- highly available, reliable, and/or scalable;
- embedded, reactive, stateful, and/or distributed.

The design patterns used in RSC make developers more productive.  They have been proven
in flagship telecom products, including (from the author's experience as its chief
software architect) the core network server that handles all of the calls in AT&T's
cellular network.  A pattern language that summarizes the patterns appears in the
[second chapter](/docs/RCS-chapter-2.pdf) of *Robust Communications Software*.  The
document [*RSC Product Overview*](/docs/RSC-Product-Overview.pdf) describes which of
them are currently available in this repository and points to the primary code files
that implement them, and this [tutorial](/docs/RCS-tutorial.pdf) provides more
information about some of them.

### POTS application

Including an application with a framework serves to test it and illustrate its use.
This repository therefore includes a POTS (Plain Ordinary Telephone Service) application.
POTS was chosen for several reasons.  For one thing, the author had extensive experience
with similar applications while working in the telecom industry.  But more importantly,
POTS is a non-trivial application, yet everyone has a reasonable understanding of what
it does.  You should therefore be able to figure out what the POTS code is doing without
reading a large specification.  An overview of the POTS application is provided
[here](/docs/RSC-POTS-Application.md).

## C++ static analysis tool

The development of RSC got sidetracked when the author decided to develop C++
static analysis tools.  This toolset detects violations of various C++ design
guidelines, such as those found in Scott Meyers' *Effective C++*.  It also analyzes
`#include` directives to determine which ones should be added or deleted.  Even if
you're not developing applications with RSC, you might find these tools useful.
An overview of them is provided [here](docs/RSC-Cpp-Static-Analysis-Tools.md).

## Installing the repository

Download the repository to a directory named `rsc`, located directly within another
directory named `rsc`.  This is because, when the executable starts, it looks for
its configuration file on the path `.../rsc/rsc/input/element.config.txt`.

## Building an executable

RSC requires C++11.

RSC is currently implemented on Windows, where it runs as a console application.
However, it defines an abstraction layer, in the form of generic C++ `.h`'s and
platform-specific `.cpp`'s, that should allow it to be ported to other systems
fairly easily.  Two executables, for 32-bit and 64-bit Windows, are provided
[here](/exe).

The directories that contain source code, and the dependencies between them, are
listed in the comments that precede the implementation of [`main`](/rsc/main.cpp).
Each of these directories is built as a separate static library, with `main`
residing in its own directory.

RSC was developed using Visual Studio 2017.  If that is also your development
environment, the `.vcxproj` (project) files in this repository should already
provide most of the build instructions that you need.  However, you will need
to change the paths to where the source code is located.  It's probably
easiest to do this by opening the `.vcxproj` files in Notepad and replacing
occurrences of `C:\Users\gregu\Documents\rsc\rsc` (the directory that contains
the source code on the author's PC) with the directory into which you downloaded
the repository.

## Running the executable

Before you run the executable, you also need to change each path (as described in
the above paragraph) in the [configuration file](input/element.config.txt), which
is read when the program is initializing during startup.

During initialization, the program displays each module as it is initialized.  (A
*module* is currently equivalent to a static library.)  After all modules have
initialized, the CLI prompt `nb>` appears to indicate that CLI commands in the
`nb` directory are available.  The information written to the console during
startup is shown [here](/output/startup.txt), and a list of all CLI commands
is provided [here](/output/help.cli.txt).

If you enter `>read saveinit` as the first CLI command, a function trace of the
initialization, which starts even before the invocation of `main`, is generated.
This trace should look a lot like [this](/output/init.trace.txt).  Each function
that appears in such a trace invoked `Debug::ft`, which records the following:
  * the function's name
  * the time when it was invoked
  * the thread that invoked it
  * its depth (in frames) on the stack: this controls indentation so that you can
see how function calls were nested
  * the total time spent in the function (in microseconds)
  * the net time spent in the function (in microseconds)

All output appears in the directory specified by `OutputPath` in the configuration file.
In addition to any specific output that you request, such as the initialization trace,
every CLI session produces
  * a `console` file (a transcript of the CLI commands that you entered and what was
written to the console)
  * a `log` file (system events that were written to the console asynchronously)
  * a `stats` file (generated periodically to report system statistics)

The numeric string *`yymmdd-hhmmss`* is appended to the names of these files to record
the time when the system initialized (for a `console` or `log` file) or when the report
was generated (for a `stats` file).

## Testing

Most of the files in the [`input`](/input) directory are test scripts.  The document that
describes the [POTS application](/docs/RSC-POTS-Application.md) also discusses its tests,
which exercise a considerable portion of the RSC software.  The tests described below are
rather tactical by comparison.

Twenty scripts test the *Safety Net* capability of the `Thread` class.  Most of these tests
cause a POSIX signal to be raised.  POSIX signals are handled by throwing a C++ exception
that is caught in `Thread.Start`, after which an appropriate recovery action is taken.
Getting the safety net to work could be a challenge when porting RSC to another
platform, which is why these tests are provided.  All of the safety net tests can be run
with the command `>read test.trap.critical.`  During each test, the following are generated
(see the `recover.*` files in the [`output`](/output) directory):

  * A function trace (`*.trace.txt`), as described above.
  * A function profile (`*.funcs.txt`) that lists each function that was invoked, along with
how many times it was invoked and the total net time spent in it.  This information is not
that useful here, but it is valuable when you want to pinpoint which functions to focus on in
order to improve real-time performance.
  * A scheduler trace (`*.sched.txt`).  The first part of this trace lists all threads in the
executable, with statistics for each.  The second part is a record of all the context switches
that occurred during the test.
  * A console file of the test (`*.cli.txt`), as described above.
 
Entering `>nt` in the CLI accesses the "nt" *increment* (a set of CLI commands).  This increment
provides sets of commands for testing functions in the [`LeakyBucketCounter`](/nb/LeakyBucketCounter.h),
[`Q1Way`](/nb/Q1Way.h), [`Q2Way`](/nb/Q2Way.h), [`Registry`](/nb/Registry.h), and
[`SysTime`](/nb/SysTime.h) interfaces.

## License

The software in this repository is freely available under the terms of the [GNU General Public
License, version 3](/LICENSE.txt).  If you have concerns about this license, it may be possible
to obtain the software under the terms of another license approved by the [Free Software
Foundation](https://www.gnu.org/licenses/license-list.html).

## Contributing

How to contribute to RSC is described [here](CONTRIBUTING.md).
