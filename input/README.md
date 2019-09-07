# CLI Scripts and Other Input Files

**NOTE**: The file [_element.config_](/input/element.config.txt) in this directory is read
during system startup to initialize various configuration parameters.  You will need to
change the value for its `SourcePath` parameter to the directory that contains all of the
downloaded RSC directories (or the directory that contains whatever source files that you
want to analyze using the C++ static analysis tools).

The file _win32_ or _win64_ is an argument to the CodeTools `>parse` command when "compiling"
in order to subsequently use C++ static analysis tools on 32-bit Windows or 64-bit Windows.
If you target RSC to another platform, you will need to create similar files for use with the
`>parse` command.  These files contain the macro names that are `#define`'d for the "compile"
(conceptually the same as defining macro names for a true compile).

The remaining files are CLI scripts.  A script is executed with the CLI command `>read <script>`.
The following table describes the scripts that you might want to run directly.

Script | Description
------ | -----------
buildlib | builds CodeTools library
debug | sets up environment before using breakpoint debugging
regression | executes all testcases and saves results in _regression.*_ files when done
restart.cold1 | initiate cold restart; use `>read restart.cold2` to capture trace
restart.warm1 | initiate warm restart; use `>read restart.warm2` to capture trace
savehelp | read this at any time to save full explanation of all CLI commands in _help.cli.txt_
saveinit | read this immediately after startup to save trace of system initialization in _init.trace.txt_ and _init.funcs.txt_
test.cp.all | executes all call processing testcases (including test.cp.setup)
test.cp.bc | executes all basic call testcases
test.cp.cfx | executes all call forwarding testcases
test.cp.cip | executes all CIP testcases (injects/verifies CIP messages using a single DN)
test.cp.cwt | executes all call waiting testcases
test.cp.setup | sets up environment for call processing testcases               
test.cp.ss | executes supplementary service testcases (BIC, BOC, HTL, SUS, WML)
test.lib.all | executes all code library testcases
test.lib.setup | sets up environment for code library testcases
test.trap.all | executes all trap testcases, which turn POSIX signals and Windows structured exceptions into C++ exceptions
test.trap.setup | sets up environment for running trap testcases
traffic.start | starts to run POTS traffic; use `>read traffic.stop` to save summary of results in _traffic.*_ files when done
