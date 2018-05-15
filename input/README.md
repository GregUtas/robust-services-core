# CLI Scripts and Other Input Files

**NOTE**: The file [`element.config`](/input/element.config.txt) in this directory is read
during system  startup to initialize various configuration parameters.  You will need to
change the values for its `HelpPath`, `InputPath`, `OutputPath`, and `SourcePath` parameters.

The file `win32` or `win64` is an argument to the CodeTools `>parse` command when "compiling"
in order to subsequently use C++ static analysis tools on 32-bit Windows or 64-bit Windows.
If you target RSC to another platform, you will need to create similar files for use with the
`>parse` command.  These files contain the macro names that are `#define`'d for the "compile"
(which has nothing to do with defining macro names for a true compile).

The remaining files are CLI scripts.  A script is executed with the CLI command `>read <script>`.
The following table describes the scripts that you might want to run directly.

Script | Description
------ | -----------
buildlib | builds CodeTools library
debug | sets up environment before using breakpoint debugging
regression | reads all testcases and saves results in `regression.*` files when done
restart.cold 1/2 | run to initiate cold restart and then to capture trace
restart.warm 1/2 | run to initiate warm restart and then to capture trace
savehelp | read this at any time to save full explanation of all CLI commands in `help.cli.txt`
saveinit | read this immediately after startup to save trace of system initialization in `init.trace.txt` and `init.funcs.txt`
test.cp.all | reads all call processing testcases (including test.cp.setup)
test.cp.bc | reads all basic call testcases
test.cp.cfx | reads all call forwarding testcases
test.cp.cip | reads all CIP testcases (injects/verifies CIP messages using a single DN)
test.cp.cwt | reads all call waiting testcases
test.cp.setup | sets up environment for call processing testcases               
test.cp.ss | reads supplementary service testcases (BIC, BOC, HTL, SUS, WML)
test.lib.all | reads all code library testcases
test.lib.setup | sets up environment for code library testcases
test.trap.critical | reads all trap testcases, with recovery thread asking to be reentered after a trap; turns POSIX signals into C++ exceptions
test.trap.non-critical | reads all trap testcases, with recovery thread asking to be exited after a trap
test.trap.setup | sets up environment for running trap testcases
traffic.start | starts to run POTS traffic; use `>read traffic.stop` to save summary of results in `traffic.*` files when done