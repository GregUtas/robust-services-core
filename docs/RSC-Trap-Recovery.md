# Robust Services Core: Trap Recovery

RSC uses the term _trap_ to refer to something, usually a POSIX signal, that
gets handled as a C++ exception. A common example is SIGSEGV, the POSIX signal
that a thread receives when it performs an illegal memory access, most often
because it used a bad pointer.

The article
[Robust C++: Safety Net](https://www.codeproject.com/Articles/5165710/Robust-Cplusplus-Safety-Net)
describes how RSC handles traps. To summarize, RSC installs a signal handler
that intercept POSIX signals and throws a C++ exception so that they can be
handled using the usual `try`-`catch` approach. However, it is _undefined
behavior_ for a signal handler to try to do almost anything useful in C++,
so the degree to which this strategy works depends on the compiler that was
used and platform for which it built the executable.

## Current Status

There are currently 28 tests that exercise RSC's Safety Net by causing traps
in various ways. RSC is currently built using the MSVC compiler, targeted to
Windows; the clang compiler, also targeted to Windows, and the gcc compiler,
targeted to Linux. The following table provides the current status of each
Safety Net test.

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
call `Thread::Kill` on this thread | KillLocal | trap.12 | pass | pass | pass
cause an infinite loop and be killed by `InitThread` | InfiniteLoop | trap.13 | pass | pass | pass
cause a stack overflow and be killed by `Thread::StackCheck` | StackOverflow | trap.14 | pass | pass | pass
trap, and trap again in `Thread::Recover` | TrapInRecover | trap.15 | pass | pass | pass
delete `Thread` object of another thread | DeleteRemote | trap.16 | pass | pass | pass
delete `Thread` object of this thread | DeleteLocal | trap.17 | pass | pass | pass
cause an infinite loop and be killed by ctrl-C | Ctrl-C  | trap.18 | pass | pass | **fail[7]**
call `Thread::EnterBlockOperation` while holding a mutex | MutexBlock | trap.19 | pass | pass | pass
trap, and trap in constructor when recreated | ThreadCtorTrap | trap.20 | pass | pass | pass
exit thread while holding a mutex | MutexExit | trap.21 | pass | pass | pass
trap while holding a mutex | MutexTrap | trap.22 | pass | pass | pass
trap, and trap once agin during trap recovery | Retrap | trap.23 | pass | pass | **fail[8]**
trap and allow thread to exit | BadPtrExit | trap.24 | pass | pass | pass
disable daemon; kill thread; reenble daemon; thread recreated | DaemonReenable | trap.25 | pass | pass | pass
exit thread; constructor traps first time daemon recreates thread; daemon disabled; reenable daemon; thread recreated | DaemonRetrap | trap.26 | pass | pass | pass
trap in destructor when exiting thread | ThreadDtorTrap | trap.27 | pass | **fail[5]** | **fail[9]**
raise SIGBUS | SIGBUS | trap.28 | n/a | n/a | pass

  1. file name in _output_ directory
  1. file name in _input_ directory
  1. causes irrecoverable stack overflow throwing exception (release build only); problem reported to Microsoft
  1. causes infinite loop dividing by zero
  1. causes irrecoverable stack overflow rethrowing exception
  1. causes irrecoverable stack overflow rethrowing exception
  1. not killed; WSL does not appear to forward ctrl-C to Linux console
  1. causes infinite loop rethrowing exception
  1. causes irrecoverable stack overflow rethrowing exception