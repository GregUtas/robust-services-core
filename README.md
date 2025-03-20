# robust-services-core

This repository contains

1. A framework for developing robust applications in C++.
1. An application built using the framework.
1. Tools for the static analysis of C++ software.
1. A framework for developing a bot that can play the board game _Diplomacy_.

## Robust Services Core

The framework for robust C++ applications is referred to as the _Robust
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
[second chapter](docs/RCS-chapter-2.pdf) of _Robust Communications Software_.
The document [_RSC Software Overview_](docs/RSC-Software-Overview.pdf)
describes which of them are currently available in this repository and points
to the primary code files that implement them, and this
[tutorial](docs/RCS-tutorial.pdf) provides more information about some of
them.

## C++ static analysis tools

The development of RSC has been somewhat sidetracked by the development of C++
static analysis tools. These tools detect violations of various C++ design
guidelines, such as those found in Scott Meyers' _Effective C++_. They also
analyze `#include` directives to determine which ones to add or delete. Their
editor then allows you to easily and interactively fix many of the warnings
that the tool generates. Even if you're not developing applications with
RSC, you might find these tools useful. An overview of them is provided
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
[here](docs/RSC-POTS-Application.md).

## Diplomacy AI client

In 2002, a group in the UK began to design a protocol that allows software
bots to play the board game
[_Diplomacy_](https://en.wikipedia.org/wiki/Diplomacy_(game)). Their
[website](http://www.daide.org.uk) has various useful links and downloads,
amongst which is the executable for a Diplomacy server. Bots log into this
server, which sends them the state of the game, allows them to communicate with
one another using the protocol, and adjudicates the moves that they submit.
Their website also provides software for developing bots. I decided to
refactor this software, decouple it from
Windows, and bring it more in line with C++11. This helped RSC evolve
to better support standalone clients that use IP (TCP, in this case). The
resulting software is available in the [_dip_](src/dip) directory and is
described in some further detail [here](docs/RSC-Diplomacy.md).

## Documentation

This page provides an overview of RSC. Another page lists
[documents](docs/README.md) that go into far more depth on many topics.

## Installing the repository

Download one of the
[releases](https://github.com/GregUtas/robust-services-core/releases/latest).
Code committed since the latest release is work in progress and may be unstable
or incomplete, so downloading from the **Code&#9662;** dropdown menu on the
main page is not recommended.

> :warning: **Warning**
>
> For proper operation, RSC must be launched from a directory below
its [_src_](src) directory. See the [installation guide](docs/Installing.md).

## Building an executable

RSC
* requires C\++17;
* is a console application;
* runs on both Windows and Linux;
* defines an abstraction layer, in the form of generic C++ _.h_'s and
platform-specific _.cpp_'s, that should allow it to be ported to other systems
fairly easily.

If you don't want to build RSC, debug and release
[executables](docs/RSC-Executables.md) are provided with each release.

The directories that contain RSC's source code, and the dependencies between
them, are listed in the comments that precede the implementation of
[`main`](src/rsc/main.cpp). Each of these directories is built as a separate
static library, with `main` residing in its own directory.

RSC is developed using Visual Studio 2022 and built using CMake, as described
[here](docs/RSC-Building-Using-CMake.md). Windows build options for RSC
are described [here](docs/RSC-Windows-Build-Options.md). Visual
Studio's _.vcxproj_ files are no longer used during builds, so
they were removed from the repository.

## Running the executable

During initialization, RSC displays each module as it is initialized.
(A _module_ is equivalent to a static library.)  After all modules
have initialized, the CLI prompt `nb>` appears to indicate that CLI commands
in the _nb_ directory are available. What is written to the console
during startup is shown [here](output/init.console.txt), and a list of all
CLI commands is provided [here](docs/help.cli.txt).

If you enter `>read saveinit` as the first CLI command, a function trace of
the initialization, which starts even before the invocation of `main`, is
generated. This trace will look a lot like [this](output/init.trace.txt).
Each function that appears in such a trace invoked `Debug::ft`, which records
the following:
  * the function's name
  * the time when it was invoked
  * the thread that invoked it
  * its depth (in frames) on the stack: this controls indentation so that you
can see how function calls were nested
  * the total time spent in the function (in microseconds)
  * the net time spent in the function (in microseconds)

All output appears in the directory _../&lt;dir>/excluded/output_, where
_&lt;dir>_ is the directory immediately above the _src_ directory.
In addition to any specific output that you request, such as the initialization
trace, every CLI session produces
  * a _console_ file (a transcript of the CLI commands that you entered and
what was written to the console)
  * a _log_ file (system events that were written to the console asynchronously)

The numeric string _yymmdd-hhmmss_ is appended to the names of these files
to record the time when the system initialized (for the _console_ file and
initial _log_ file) or the time of the most recent restart (for a subsequent
_log_ file).

## Developing an application

The easiest way to use RSC as a framework is to create a static library below
RSC's _src_ directory. The [_app_](src/app) directory is provided for
this purpose. Simply use whatever subset of RSC that your application
needs. This will always include the namespace `NodeBase` (the [_nb_](src/nb)
directory). It might also include `NetworkBase` (the [_nw_](src/nw)
directory) and `SessionBase` (the [_sb_](src/sb) directory). Using a new
namespace for your application is recommended.

If you put your code elsewhere, RSC will be unable to find important
directories when you launch it, as described in the
[installation guide](docs/Installing.md). You will then need
to modify the function [`Element::RscPath`](src/nb/Element.cpp) so that it
can find the directory that contains the [_input_](input) directory. You
should also add RSC's [_help_](help) directory to that directory.

To initialize your application, derive from [`Module`](src/nb/Module.h).
For an example, see [`NbModule`](src/nb/NbModule.cpp), which initializes
`NodeBase`. Change [`CreateModules`](src/app/rscapp.cpp) so that it also
instantiates your module, as well as the other modules that you need in
your build.

To interact with your application, derive from
[`CliIncrement`](src/nb/CliIncrement.h).
For an example, see [`NbIncrement`](src/nb/NbIncrement.cpp), the increment
for `NodeBase`. Instantiate your CLI increment in your module's `Startup`
function. When you launch RSC, you can then access the commands in your
increment through the CLI by entering `>incr`, where `incr` is the
abbreviation that your increment's constructor passed to `CliIncrement`'s
constructor.

## Testing

Most of the files in the [_input_](input) directory are test scripts. The
document that describes the [POTS application](docs/RSC-POTS-Application.md)
also discusses its tests, which exercise a considerable portion of RSC's
software. Some other tests are more tactical in nature:

- A set of scripts tests the Safety Net capability of the `Thread` class.
A dedicated [page](docs/RSC-Trap-Recovery.md) describes these tests and the
current status of each one.
 
- Entering `>nt` in the CLI accesses the "nt" _increment_ (a set of CLI
commands). It provides commands for testing functions in the
[`BuddyHeap`](src/nb/BuddyHeap.h),
[`SlabHeap`](src/nb/SlabHeap.h),
[`LeakyBucketCounter`](src/nb/LeakyBucketCounter.h),
[`Q1Way`](src/nb/Q1Way.h), [`Q2Way`](src/nb/Q2Way.h), and
[`Registry`](src/nb/Registry.h) interfaces.

## Licensing

RSC is freely available under the terms of the [Lesser GNU General Public
License, version 3](LICENSE.md), which basically says that you must also
publish your changes to RSC, such as bug fixes and enhancements. If you
are developing commercial software that you want to keep proprietary, the
LGPLv3 license allows this.

## Contributing

How to contribute to RSC is described [here](CONTRIBUTING.md).

## Sponsoring

GitHub now lets you sponsor projects. A "Sponsor" button is located at the top
of this page.

