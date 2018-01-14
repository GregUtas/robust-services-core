# CLI Documentation

This directory contains supplementary help files for CLI increments.  Only the CLI
itself, and the `>ct` increment and its `>export` and `>parse` commands, currently
have such help files.  However, the CLI framework ensures that each command provides
basic help information.

* `>help full` writes `cli.txt` to the console.
* `>help <incr> full` causes the file `cli.<incr>.txt` to be written to the console.
For example, `>help ct full` writes `cli.ct.txt` to the console.
* `>help <comm> full` causes the file `cli.<incr>.<comm>.txt` to be written to the
console.  For example, `>help parse full` writes `cli.ct.parse.txt` to the console.
