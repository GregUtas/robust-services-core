# robustness

This repository contains the following:
1. A framework for developing robust services (applications) in C++.
2. An application built using the framework.
3. A set of tools for the static analysis of C++ software.

## License
All of the software in this repository was developed by Greg Utas, who
currently retains copyright.

## Robust Services Core
The framework that supports robust applications is referred to as the
*Robust Services Core* (RSC).  Many of the patterns in RSC are used in
telecom products, where the term *carrier-grade* refers to a server with
five- or six-nines availability.  A pattern language which summarizes these
patterns is found in the second
chapter of *Robust Communications Software*, which is located in the docs/
directory.  The document *RSC Product Overview* in that directory describes
which of these patterns are currently available in this repository and the
primary code files that implement them.

RSC is currently targeted at Windows.  It defines an abstraction layer, in
the form of common C++ `.h`'s and platform-specific `.cpp`'s, that should
allow it to be ported to other systems without much difficulty.

RSC uses a few C++11 capabilities, primarily the `unique_ptr` template and
the `override` tag.

## POTS Application
Including an application with a framework serves to test it and illustrate its
use.  For this purpose, this repository includes a POTS (Plain Ordinary Telephone
Service) application.  POTS was chosen for several reasons.
First, the author had extensive experience with such applications while working
as a software architect in the telecom industry.  But more importantly, POTS
is a non-trivial application, and everyone has a reasonable understanding of what
it does.  You should therefore be able to understand what the POTS code is doing
without first having to read a large specification.  An overview of the POTS
application is provided in *RSC POTS Application* in the docs/ directory.

## C++ Static Analysis Tool
The development of RSC got sidetracked when the author decided to
develop a C++ static analysis tool.  The tool detects violations of various C++
design guidelines (for example, the types of guidelines found in Scott Myer's
*Effective C++*).  The tool also analyzes `#include` lists to determine which
`#include`'s should be added or removed.  An overview of the tool is provided
in *RSC C++ Static Analysis Tool* in the docs/ directory.
