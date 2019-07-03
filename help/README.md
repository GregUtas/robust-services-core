# Documentation for CLI increments, logs, and alarms

This directory contains additional help for CLI increments.  Only the CLI itself,
as well as the `>ct` increment and a handful of commands, currently have additional
help.  However, the CLI framework ensures that each command provides basic help
information.

* `>help full` provides an overview of the CLI.
* `>help <incr> full` provides additional help for a specific increment.
* `>help <comm> full` provides additional help for a specific command.

Documentation for each log and alarm is also located in this directory.  Each help
file contains keys, which are lines that begin with a '$'.  Each key precedes the
additional help for a particular topic, which ends at the next key.
