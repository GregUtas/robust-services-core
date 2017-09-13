# robustness

This repository contains
1. A framework for developing robust applications in C++.
2. An application built using the framework.
3. Tools for the static analysis of C++ software.

## License
All of the software in this repository was developed by Greg Utas, who
currently retains copyright.

## Robust Services Core
The framework that supports robust applications is referred to as the
*Robust Services Core* (RSC).  Many of the design patterns in RSC are used
in telecom products, where the term *carrier-grade* refers to a server with
at least five-nines (99.999%) availability.  A pattern language summarizing
these patterns appears in the second
chapter of *Robust Communications Software*, which is included in the [`docs`](/docs)
directory.  The document [*RSC Product Overview*](/docs/RSC-Product-Overview.pdf)
describes which of these patterns are currently available in this repository
and the primary code files that implement them.

RSC is currently targeted at Windows.  It defines an abstraction layer, in
the form of common C++ `.h`'s and platform-specific `.cpp`'s, that should
allow it to be ported to other systems fairly easily.

RSC uses a few C++11 capabilities, primarily the `unique_ptr` template and
the `override` tag.

## POTS Application
Including an application with a framework serves to test it and illustrate its
use.  To these ends, this repository includes a POTS (Plain Ordinary Telephone
Service) application.  POTS was chosen for several reasons.  For one thing,
the author had extensive experience with such applications while working
as a software architect in the telecom industry.  But more importantly, POTS
is a non-trivial application, and everyone has a reasonable understanding of what
it does.  You should therefore be able to understand what the POTS code is doing
without having read a large specification.  An overview of the POTS application
is provided [here](/docs/RSC-POTS-Application).

## C++ Static Analysis Tool
The development of RSC got sidetracked when the author decided to develop C++
static analysis tools.  This toolset detects violations of various C++ design
guidelines, such as those found in Scott Myer's *Effective C++*.  It also analyzes
`#include` lists to find `#include`'s that should be added or deleted.  An
overview of the toolset is provided [here](docs/RSC-Cpp-Static-Analysis-Tools).
