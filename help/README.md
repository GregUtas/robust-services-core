# Documentation for CLI increments, logs, alarms, and static analysis warnings

This directory contains additional help for CLI increments. Only the CLI
itself, as well as the `>ct` increment and a handful of commands, currently
have additional help. However, the CLI framework ensures that each command
provides help for each of its parameters.

* `>help full` provides an overview of the CLI.
* `>help <incr> full` provides additional help for a specific increment.
* `>help <comm> full` provides additional help for a specific command.

Documentation for each log, alarm, and static analysis warning is also located
in this directory. Their help files contain keys, which are lines that begin
with `?`. Each key precedes the additional help for a particular topic, which
ends with the next key. A key that ends with `*` matches a lookup that begins
with the string that precedes the `*`.
