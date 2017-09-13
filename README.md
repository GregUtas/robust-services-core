# robustness

This repository contains the following:
1. A framework for constructing robust services (applications).
2. An application built using the framework.
3. A set of tools for the static analysis of C++ software.

*Robust Services Core*
The framework is referred to as the /Robust Services Core/ (RSC).  Many
of the patterns used in RSC were used in the telecom industry, where the
term /carrier-grade/ refers to servers with five- or six-nines availability.
A pattern language that summarizes these patterns is found in the second
chapter of /Robust Communications Software/, which is included in the docs/
directory.  The /RSC Product Overview/ document in that directory describes
which of these patterns are currently implemented in this repository and the
primary code files that embody them.

*POTS Application*
Including a application with a framework helps to test it and illustrate how
to use it.  For this purpose, a POTS (Plain Ordinary Telephone Service) is
included in the repository.  This application was chosen for several reasons.
For one thing, the author had extensive experience with such applications during
his career as a software architect in the telecom sector.  More importantly, POTS
is a non-trivial application, and everyone has a reasonable understanding of what
it does.  You should therefore be able to understand what the POTS code is doing
without having to read a large specification.
