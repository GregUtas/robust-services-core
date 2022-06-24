# Robust Services Core: Trap Recovery

In RSC, the term _trap_ refers to something, usually a POSIX signal, that
results in a C++ exception. A common example is SIGSEGV, the signal that a
thread receives when it performs an illegal memory access, perhaps because
it used a bad pointer.

[Robust C++: Safety Net](https://www.codeproject.com/Articles/5165710/Robust-Cplusplus-Safety-Net)
describes how RSC handles traps. To summarize, RSC installs a handler that
intercepts POSIX signals and throws a C++ exception so that a signal can be
handled by a `catch` clause in [`Thread::Start`](/src/nb/Thread.cpp), the
function that underlies all RSC threads. However, it is _undefined behavior_
for a signal handler to try to do almost anything useful in C++, so how well
this strategy works depends on the compiler that was used and the platform
for which it targeted the executable.

RSC has 28 tests that tell [`RecoveryThread`](/src/nt/NtIncrement.cpp)
to cause traps in various ways. Getting the safety net to work could be
challenging when porting RSC to another platform, which is one reason
why these tests are provided. The entire set can be run with the command
`>read test.trap.all.` Each test generates the following (see the
_recover.*_ files in the  [_output_](output) directory):

  * A function trace (_*.trace.txt_).
  * A function profile (_*.funcs.txt_) that lists each function that was
invoked, along with how many times it was invoked and the total net time
spent in it. This information is not that useful here, but it is valuable
when you want to pinpoint which functions to focus on in order to improve
real-time performance.
  * A scheduler trace (_*.sched.txt_). The first part of this trace lists
allthreads in the executable, with statistics for each. The second part is
a record of all the context switches that occurred during the test.
  * A console file of the test (_*.cli.txt_).
 
## Current Status

RSC is built using

- the MSVC compiler, targeted to Windows;
- the clang compiler, also targeted to Windows; and
- the gcc compiler, targeted to Linux.

The following table provides the current status of each safety net test for
the above combinations.

Description | Test Name[1] | Script[2] | MSVC/Windows | clang/Windows | gcc/Linux
----------- | ------------ | --------- | ------------ | ------------- | ---------
exit thread | Return | trap.01 | pass | pass | pass
throw an exception | Exception | trap.02 | pass | pass | pass
use a bad pointer | BadPtr | trap.03 | pass | pass | pass
divide by zero | DivZero | trap.04 | pass | **fail[4]** | pass
raise `SIGINT` | SIGINT | trap.05 | pass | pass | pass
raise `SIGILL` | SIGILL | trap.06 | pass | pass | pass
raise `SIGTERM` | SIGTERM | trap.07 | pass | pass | pass
raise `SIGBREAK` | SIGBREAK | trap.08 | pass | pass | n/a
call `abort` | abort | trap.09 | pass | pass | pass
call `std::terminate` | terminate | trap.10 | **fail[3]** | pass | **fail[6]**
call `Thread::Kill` on another thread | KillRemote | trap.11 | pass | pass | pass
call `Thread::Kill` on running thread | KillLocal | trap.12 | pass | pass | pass
cause an infinite loop and be killed by `InitThread` | InfiniteLoop | trap.13 | pass | pass | pass
cause a stack overflow and be killed by `Thread::StackCheck` | StackOverflow | trap.14 | pass | pass | pass
trap, and trap again in `Thread::Recover` | TrapInRecover | trap.15 | pass | pass | pass
delete `Thread` object of another thread | DeleteRemote | trap.16 | pass | pass | pass
delete `Thread` object of running thread | DeleteLocal | trap.17 | pass | pass | pass
cause an infinite loop and be killed by **ctrl-C** | Ctrl-C  | trap.18 | pass | pass | **fail[7]**
call `Thread::EnterBlockingOperation` while holding a mutex | MutexBlock | trap.19 | pass | pass | pass
trap, and constructor traps first time thread is recreated | ThreadCtorTrap | trap.20 | pass | pass | pass
exit thread while holding a mutex | MutexExit | trap.21 | pass | pass | pass
trap while holding a mutex | MutexTrap | trap.22 | pass | pass | pass
trap, and trap once more during trap recovery | Retrap | trap.23 | pass | pass | **fail[8]**
trap and allow thread to exit | BadPtrExit | trap.24 | pass | pass | pass
disable [`Daemon`](/src/nb/Daemon.h); kill thread; reenable `Daemon`; thread recreated | DaemonReenable | trap.25 | pass | pass | pass
exit thread; constructor traps first time `Daemon` recreates thread, so `Daemon` is disabled; reenable `Daemon`; thread recreated | DaemonTrap | trap.26 | pass | pass | pass
trap in destructor when exiting thread | ThreadDtorTrap | trap.27 | pass | **fail[5]** | **fail[9]**
raise `SIGBUS` | SIGBUS | trap.28 | n/a | n/a | pass

  1. file name in [_output_](/output) directory
  1. file name in [_input_](/input) directory
  1. causes stack overflow by rethrowing exceptions (release build only)
  1. causes infinite loop that divides by zero
  1. causes stack overflow by rethrowing exceptions
  1. causes stack overflow by rethrowing exceptions
  1. not killed; WSL does not appear to forward ctrl-C to Linux console
  1. causes infinite loop that rethrows exceptions
  1. causes stack overflow by rethrowing exceptions
