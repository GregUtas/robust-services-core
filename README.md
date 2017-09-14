# robustness

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

## POTS Application
Including an application with a framework serves to test it and illustrate its
use.  To these ends, this repository includes a POTS (Plain Ordinary Telephone
Service) application.  POTS was chosen for several reasons.  For one thing,
the author had extensive experience with such applications while working
as a software architect in the telecom industry.  But more importantly, POTS
is a non-trivial application, and everyone has a reasonable understanding of what
it does.  You should therefore be able to figure out what the POTS code is doing
without having read a large specification.  An overview of the POTS application
is provided [here](/docs/RSC-POTS-Application.md).

## C++ Static Analysis Tool
The development of RSC got sidetracked when the author decided to develop C++
static analysis tools.  This toolset detects violations of various C++ design
guidelines, such as those found in Scott Myer's *Effective C++*.  It also analyzes
`#include` lists to find `#include`'s that should be added or deleted.  An overview
of the toolset is provided [here](docs/RSC-Cpp-Static-Analysis-Tools.md).

## Building an Executable
RSC is primarily implemented in C++0x.  However, it uses some C++11 capabilities,
primarily the `unique_ptr` template and the `override` tag.

RSC is currently implemented on Windows, where it runs as a basic console
application.  However, it defines an abstraction layer, in the form of generic
C++ `.h`'s and platform-specific `.cpp`'s, that should allow it to be ported
to other systems fairly easily.

The directories that contain source code, and their dependencies, are listed in
the comments that precede the implementation of [`main`](/rsc/main.cpp).  RSC is
currently developed using Visual Studio 2017.  If this is also your development
environment, the `.vcxproj` (project) files in this repository already contain
most of the build instructions that you need.  The main thing that you need to
do is change the paths to where the source code is located.  It's probably
easiest to do this by opening the `.vcxproj` files in Notepad and replacing
occurrences of `C:\Users\gregu\Documents\rsc\rsc` (the directory that contains the
source code on my PC) with the top-level directory into which you downloaded the
repository.

Before running the executable, you also have to modify paths in the
[configuration file](input/element.config.txt), which is read during startup.
