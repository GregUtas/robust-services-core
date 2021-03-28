# robust-services-core

This repository contains

1. A framework for developing robust applications in C++.
1. An application built using the framework.
1. Tools for the static analysis of C++ software.
1. A framework for developing a bot that can play the board game _Diplomacy_.

## Robust Services Core

The framework that supports robust applications is referred to as the _Robust
Services Core_ (RSC). RSC will put your project on the right path and jump-start
it if you're developing or reengineering a system whose requirements can be
characterized as

- highly available, reliable, and/or scalable;
- embedded, reactive, stateful, and/or distributed.

The design patterns used in RSC make developers more productive. They have
been proven in flagship telecom products, including (from the author's
experience as its chief software architect) the core network server that
handles all of the calls in AT&T's cellular network. A pattern language that
summarizes the patterns appears in the
[second chapter](/docs/RCS-chapter-2.pdf) of _Robust Communications Software_.
The document [_RSC Software Overview_](/docs/RSC-Software-Overview.pdf)
describes which of them are currently available in this repository and points
to the primary code files that implement them, and this
[tutorial](/docs/RCS-tutorial.pdf) provides more information about some of
them.

## C++ static analysis tools

The development of RSC has been somewhat sidetracked by the development of C++
static analysis tools. These tools detect violations of various C++ design
guidelines, such as those found in Scott Meyers' _Effective C++_. They also
analyze `#include` directives to determine which ones to add or delete. Their
editor then allows you to easily and interactively fix about half of the 130
or so warning types.  Even if you're not developing applications with RSC, you
might find these tools useful. An overview of them is provided
[here](docs/RSC-Cpp-Static-Analysis-Tools.md).

## POTS application

Including an application with a framework serves to test it and illustrate its
use. This repository therefore includes a POTS (Plain Ordinary Telephone
Service) application. POTS was chosen for several reasons. For one thing, the
author had extensive experience with similar applications while working in the
telecom industry. But more importantly, POTS is a non-trivial application, yet
everyone has a reasonable understanding of what it does. You should therefore
be able to figure out what the POTS code is doing without reading a large
specification. An overview of the POTS application is provided
[here](/docs/RSC-POTS-Application.md).

## Diplomacy AI client

In 2002, a group in the UK began to design a protocol that allows software
bots to play the board game
[_Diplomacy_](https://en.wikipedia.org/wiki/Diplomacy_(game)). See their
[website](http://www.daide.org.uk) for various useful links and downloads,
amongst which is the executable for a Diplomacy server. Bots log into this
server, which sends them the state of the game, allows them to communicate with
one another using the protocol, and adjudicates the moves that they submit.
Their website also provides base software for developing bots. It seemed that
it would be interesting to refactor this software while decoupling it from
Windows and bringing it more in line with C++11. This would help RSC evolve
to better support standalone clients that use IP (TCP, in this case). The
resulting software is available in the [_dip_](/dip) directory and is
described in some further detail [here](/docs/RSC-Diplomacy.md).

## Documentation

This page provides an overview of RSC. There is also a page which lists
[documents](/docs/README.md) that go into far more depth on many topics.

## Installing the repository

After you download and extract the repository, its top-level directory will have
a name like _robust-services-core-v0.n.n_ now that releases are tagged. So will
the directory directly underneath it. Rename that second-level directory _rsc_.
This is because, when the executable starts, it looks for its
[configuration file](input/element.config.txt) on the path
_../rsc/input/element.config.txt_.

## Building an executable

RSC requires C++11.

RSC is currently implemented on Windows, where it runs as a console application.
However, it defines an abstraction layer, in the form of generic C++ _.h_'s and
platform-specific _.cpp_'s, that should allow it to be ported to other systems
fairly easily. Debug and release executables, for 32-bit and 64-bit Windows, are
provided [here](/exe).

The directories that contain RSC's source code, and the dependencies between
them, are listed in the comments that precede the implementation of
[`main`](/rsc/main.cpp). Each of these directories is built as a separate
static library, with `main` residing in its own directory.

RSC is currently developed using Visual Studio 2017. If that is also your
development environment, the _.vcxproj_ (project) files in this repository
should already provide most of the build instructions that you need. The
Windows build options used for RSC are described
[here](/docs/RSC-Windows-Build-Options.md).

## Running the executable

During initialization, the program displays each module as it is initialized.
(A _module_ is currently equivalent to a static library.)  After all modules
have initialized, the CLI prompt `nb>` appears to indicate that CLI commands
in the _nb_ directory are available. The information written to the console
during startup is shown [here](/output/init.console.txt), and a list of all
CLI commands is provided [here](/docs/help.cli.txt).

If you enter `>read saveinit` as the first CLI command, a function trace of
the initialization, which starts even before the invocation of `main`, is
generated. This trace should look a lot like [this](/output/init.trace.txt).
Each function that appears in such a trace invoked `Debug::ft`, which records
the following:
  * the function's name
  * the time when it was invoked
  * the thread that invoked it
  * its depth (in frames) on the stack: this controls indentation so that you
can see how function calls were nested
  * the total time spent in the function (in microseconds)
  * the net time spent in the function (in microseconds)

All output appears in the directory _../rsc/excluded/output_ (the first
directory named _rsc_ above the directory from which the _.exe_ was launched).
In addition to any specific output that you request, such as the initialization
trace, every CLI session produces
  * a _console_ file (a transcript of the CLI commands that you entered and
what was written to the console)
  * a _log_ file (system events that were written to the console asynchronously)

The numeric string _yymmdd-hhmmss-mmm_ is appended to the names of these files
to record the time when the system initialized (for the _console_ file and
initial _log_ file) or the time of the preceding restart (for a subsequent
_log_ file).

## Testing

Most of the files in the [_input_](/input) directory are test scripts. The
document that describes the [POTS application](/docs/RSC-POTS-Application.md)
also discusses its tests, which exercise a considerable portion of the RSC
software. The tests described below are rather tactical by comparison.

Twenty-seven scripts test the _Safety Net_ capability of the `Thread` class.
Most of them cause a POSIX signal to be raised. POSIX signals are handled by
throwing a C++ exception that is caught in `Thread.Start`, after which an
appropriate recovery action is taken. Getting the safety net to work could be
challenging when porting RSC to another platform, which is why these tests are
provided. All of the safety net tests can be run with the command
`>read test.trap.all.`  During each test, the following are generated (see
the _recover.*_ files in the [_output_](/output) directory):

  * A function trace (_*.trace.txt_), as described above.
  * A function profile (_*.funcs.txt_) that lists each function that was
invoked, along with how many times it was invoked and the total net time spent
in it. This information is not that useful here, but it is valuable when you
want to pinpoint which functions to focus on in order to improve real-time
performance.
  * A scheduler trace (_*.sched.txt_). The first part of this trace lists all
threads in the executable, with statistics for each. The second part is a
record of all the context switches that occurred during the test.
  * A console file of the test (_*.cli.txt_), as described above.
 
Entering `>nt` in the CLI accesses the "nt" _increment_ (a set of CLI
commands). It provides sets of commands for testing functions in the
[`NbHeap`](/nb/NbHeap.h), [`LeakyBucketCounter`](/nb/LeakyBucketCounter.h),
[`Q1Way`](/nb/Q1Way.h), [`Q2Way`](/nb/Q2Way.h), [`Registry`](/nb/Registry.h),
and [`SysTime`](/nb/SysTime.h) interfaces.

## Licensing

RSC is freely available under the terms of the [GNU General Public License,
version 3](/LICENSE.txt), which basically says that you must also publish your
own software, as well as your changes to RSC. If you are developing software
that you want to keep proprietary, the GPLv3 license also allows RSC to be
licensed under the terms of another license approved by the [Free Software
Foundation](https://www.gnu.org/licenses/license-list.html). Some of those
licenses allow your software to remain proprietary. However, compensation
would be required, and any changes that you make to RSC itself (bug fixes
and enhancements) would still have to be published.

## Contributing

How to contribute to RSC is described [here](CONTRIBUTING.md).

## Sponsoring

GitHub now lets you sponsor projects. A "Sponsor" button is located at the top
of this page.
