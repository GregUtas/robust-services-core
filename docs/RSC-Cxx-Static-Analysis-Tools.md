# C++ Static Analysis Tools

The C++ static analysis tools are accessed in the `ct` increment, which
is available after entering the CLI command `>ct`.  See the [list of CLI
commands](/docs/output/help.cli.txt) for commands available in the `ct`
increment.  They are also available by entering `>help ct` (for a brief
summary) or `>help ct full` (for a more detailed explanation).

Before using any of the commands, the contents of the code library must be
defined.  This is done by entering `>read buildlib`.

As the library is built, `#include` relationships are noted.  This allows
`#include` dependencies to be analyzed by the operators `us`, `ub`, `as`,
`ab`, `ca`, `ns`, and `nb`, which are described in the full help documentation.

Some commands cannot be used until the code has also been parsed.  To parse
all of the files in the library, enter `>parse - $files`.  If a file has not
been parsed, it will first be parsed if you enter a command that requires this.
Commands that require parsing include

* `>check`, to check for violations of C++ design guidelines
* `>export`, to export the code using standardized formatting
* `>trim`, to determine if an `#include` should be added or removed
* `>apply`, to modify each file using the output from `>trim` 
