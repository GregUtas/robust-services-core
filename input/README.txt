/ element.config: element configuration file: UPDATE InputPath and OutputPath
/ script.name: CLI symbol for name of script ("regression" or "traffic")
/
/ Run a script from the CLI using >read <script>
/
/ SCRIPT                  DESCRIPTION
/ buildlib                builds code library
/ debug                   executes "cfgparms set breakenabled t" to suppress
/                         scheduling timeout log
/ finish                  saves traffic, scheduler, and statistics reports in
/                         the file defined by script.name
/ regression              reads all testcases and saves results in regression.*
/                         files when done
/ restart.cold 1/2        run to initiate cold restart and then to capture trace
/ restart.warm 1/2        run to initiate warm restart and then to capture trace
/ savehelp                read this at any time to save full explanation of all
/                         CLI commands in help.cli.txt
/ saveinit                read this immediately after startup to save trace of
/                         system initialization in init.trace and init.funcs.txt
/ test.cp.all             reads all call processing testcases
/ test.cp.bc              reads all basic call testcases
/ test.cp.cfx             reads all call forwarding testcases
/ test.cp.cip             reads all CIP testcases (injects/verifies CIP messages
                          using a single DN)
/ test.cp.cwt             reads all call waiting testcases
/ test.cp.epilog          automatically read after each call processing testcase
/ test.cp.prolog          automatically read before each call processing testcase
/ test.cp.setup           sets up environment for call processing testcases               
/ test.cp.ss              reads supplementary service testcases
/                         (BIC, BOC, HTL, SUS, WML)
/ test.lib.all            reads all code library testcases
/ test.lib.epilog         automatically read after each code library testcase
/ test.lib.prolog         automatically read before each code library testcase
/ test.lib.setup          sets up environment for code library testcases
/ test.trap.critical      reads all trap testcases, with recovery thread asking
/                         to be reentered after a trap; turns Posix signals into
/                         C++ exceptions
/ test.trap.epilog        automatically read after each trap testcase
/ test.trap.non-critical  reads all trap testcases, with recovery thread
/                         asking to be exited after a trap
/ test.trap.prolog        automatically read before each trap testcase
/ test.trap.setup         sets up environment for running trap testcases
/ traffic                 starts to run traffic; use ">read finish" to save
/                         summary of results in traffic.* files when done