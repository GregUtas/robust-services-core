# Robust Services Core: Coding Guidelines
All code must `>parse` successfully. If it doesn't, try to use a construct that the parser understands.
Failing that, enhance the parser (or ask for it to be enhanced) to support the missing language feature.

Use the `>check` command to help determine whether software follows recommended guidelines. It supports
many of the guidelines in this document.

The existing software does not always follow every guideline. In some cases, there is a good reason
for violating a guideline. In others, the effort that would be needed to make the software conform
is better spent elsewhere. Some of the software was developed before C++11, so there are things that
it should still adopt (e.g. greater use of `unique_ptr`).

## Formatting
Try to make it impossible for the reader to tell where code was added or changed.
1. Begin each file with the following heading:
```
//================================================================================
//
//  <FileName.ext>
//
//  Copyright (C) 2017  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later
//  version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with RSC.  If not, see <http://www.gnu.org/licenses/>.
```
2. Use spaces instead of tabs.
1. Indent a multiple of 3 spaces.
1. Remove unnecessary interior spaces and semicolons. Remove trailing spaces.
1. Use `//` comments instead of `/*...*/`.
1. Add blank lines for readability, but avoid multiple blank lines.
1. Limit lines to 80 characters in length.  Break after `,:)` and before `(`. A break at an operator can
either occur before or after, depending on which reads better.
1. Break before `{` and `}` unless everything in between also fits on the same line.
1. Almost always use Camel case. Use all uppercase and interior underscores only in
low-level types and constants. Names that evoke Hungarian notation are an abomination.
1. Keep `*` and `&` with the type instead of the variable (`Type* t` instead of `Type *t`).

## Interfaces
1. Insert an `#include` guard based on the file name (_FileName.h_ and `FILENAME_H_INCLUDED`) immediately
after the standard heading.
1. Sort `#include` directives as follows:
   1. header(s) that declare something that this .cpp defines
   1. header(s) that defines base class of classes defined in this file
   1. external headers (#include <filename>)
   1. interal headers (#include "filename.h")
   And alphabetically within each group.
1. Remove an `#include` solely associated with functions inherited from a base class.
1. Remove an `#include` by forward-declaring a class that is only named in references or pointers. Use
an explicit forward declaration instead of relying on this as a side effect of a friend declaration.
1. Avoid `using` declarations and directives for `std` symbols. Prefix the namespace directly (i.e.
`std::<symbol>`).
1. Initialize global data (static members) in the _.cpp_ if possible.

## Preprocessor
Do not use the preprocessor except for one of the following purposes:
1. An `#include` guard.
1. Conditional compilation (`#ifdef`). Symbols used here are defined when launching the compiler. Those
in current use are
   1. `OS_WIN` for Windows (defines a specific platform: may only be used in platform-specific files;
   those for Windows are named _*.win.cpp_)
   1. `FIELD_LOAD` for a production build (else assumed to be a debug build; may only be used in a _.cpp_
   that executes _before_ the configuration file has been read during system initialization; otherwise
   use `Element::RunningInLab()`)
   1. `WORDSIZE_32` for a 32-bit CPU (else assumed to be 64-bit; may only be used in _subs/_ files)
   1. `CT_COMPILER` when running the `>parse` command (may only be used in _subs/_ files) </li>
1. To `#define` an imitation keyword that maps to an empty string. The only current example is `NO_OP`.

## Implementations
1. Sort `#include` directives alphabetically within the following groups:
   1. header(s) that declare something that this _.cpp_ defines
   1. header(s) that define direct base class of classes defined in this file
   1. external headers (`#include <filename>`)
   1. interal headers (`#include "filename.h"`)
1. Omit any `#include` or using that is already in the header.
1. Put all of the code in the same namespace as the class defined in the header.
1. Implement functions in alphabetical order, following the constructor(s) and destructor.
1. Separate functions in the same class with a `//----`... that is 80 characters long.
1. Separate private classes (local to the .cpp) with a `//====`... that is 80 characters long.
1. Add a blank line after the `fn_name` that defines a function’s name for `Debug::ft`.
1. Name constructors and destructors `"<class>.ctor"` and `"<class>.dtor"` for `Debug::ft`.
1. Fix compiler warnings through level 4.

## Classes
1. Give a class its own _.h_ and _.cpp_ unless it is trivial, closely related to others, or private to an implementation.
1. A base class should be abstract. Its constructor should therefore be protected.
1. Tag a constructor as `explicit` if it can be invoked with one argument.
1. Make each public function non-virtual, with a one-line invocation of a virtual function if necessary.
1. Make each virtual function private if possible, or protected if derived classes may need to invoke it.
1. Make a base class destructor
   1. virtual and public
   1. non-virtual and protected
   1. virtual and protected, to restrict deletion
1. If a destructor frees a resource, even automatically through a `unique_ptr` member, also define
   - a copy constructor: `Class(const Class& that);`
   - a copy assignment operator: `Class& operator=(const Class& that);` </li>
   Here, create copies of `that`’s resources first, then release `this`’s existing resources, and finally assign
   the new ones.
1. If a class allows copying, also define "move" functions. These release `this`’s existing resources and then take
over the ones owned by `that`. Their implementations typically use `std::swap`.
   - a move constructor: `Class(Class&& that);`
   - a move assignment operator: `Class& operator=(Class&& that);`
1. In C++11, each of the above functions can be suffixed with `= delete` to prohibit its use, or `= default` to use
the compiler-generated default.
1. To prohibit stack allocation, delete the constructor and/or destructor.
1. To prohibit scalar heap allocation, delete `operator new`.
1. To prohibit vector heap allocation, delete `operator new[]`.
1. If a class only has static members, convert it to a namespace. If this is not possible, delete its constructor.
1. Use `override` when overriding a function defined in a base class.
1. Within the same level of access control, sort overridden functions alphabetically.
1. Make a function or argument `const` when appropriate.
1. Remove `inline` as a keyword.
1. Avoid `friend` where possible.
1. Override `Display` if a class has data.
1. Override `Patch` except in a trivial leaf class.
1. Avoid invoking virtual functions in the same class hierarchy within constructors and destructors.
1. Provide an implementation for a pure virtual function to highlight the bug of calling it too early during
construction or too late during destruction.
1. If a class is large, consider using the PIMPL idiom to move its private members to the _.cpp_.
1. When only a subset of a class’s data should be write-protected, split it into a pair of collaborating classes
that use `MemProt` and `MemDyn` (or `MemImm` and `MemPerm`).
1. Static member data begins with an uppercase letter and ends with an underscore, which may be omitted if it is
not returned by a "Get" function. Non-static member data begins with a lowercase letter and ends with an underscore,
which may be omitted if it is public (in which case it should probably be in a `struct`).

## Functions
1. Use the initialization list for constructors. Initialize members in the order that the class declared them.
1. Use `()` instead of `(void)` for an empty argument list.
1. Name each argument. Use the same name in the interface and the implementation.
1. Make the invocation of `Debug::ft` the first line in a function body, and follow it with a blank line.
1. The `fn_name` passed to `Debug::ft` and other functions must accurately reflect the name of the invoking function.
1. A simple "Get" function should not invoke `Debug::ft` unless it is virtual.
1. Left-align the types and variable names in a long declaration list.
1. Use `nullptr` instead of `NULL`.
1. Check for `nullptr`, even when an argument is passed by reference. (A reference merely _documents_ that
`nullptr`is an invalid input.)
1. After invoking `delete`, set a pointer to `nullptr`.
1. Use `unique_ptr` to avoid the need for `delete`.
1. Use `unique_ptr` so that a resource owned by a stack variable will be freed if an exception occurs.
1. Declare loop variables inline (e.g. `for auto i =`).
1. Declare variables of limited scope inline, as close to where they are used as is reasonable.
1. Use `auto` unless specifying the type definitely improves readability.
1. Include `{`...`}` in all non-trivial `if`, `for`, and `while` statements.
1. When there is an `else`, use braces in both or neither clauses of the `if`.
1. When there is no `else`, use braces for the statement after `if` unless it fits on the same line.
1. Define string constants in a common location to support future localization.
1. To force indentation, even in the face of automated formatting, use `{`...`}` between function pairs such as
   1. `EnterBlockingOperation` and `ExitBlockingOperation`
   1. `Lock` and `Unlock`
   1. `MakePreemptable` and `MakeUnpreemptable`
1. It is a serious bug for a function to cause an unexpected exception, so check arguments and results returned
by other functions.
1. Throw an exception only when it is impossible to fail gracefully or the work being performed must be aborted.
Prefer to generate a log (`Debug::SwLog`) and return a failure value.
   
## Tagged comments
Some comments identify work items. They have the form `//a`, where `a` is some character.  The
following are currently used:
- `//&` is something in [`main.cpp`](/rsc/main.cpp) that might be enabled for a subset build
- `//>` is an internal constant that can be changed to alter behavior
- `//@` is a useful breakpoint during development
- `//b` is a basic call enhancement
- `//c` is a `CodeTools` enhancement
- `//d` is a decoupling enhancement
- `//e` is an unclassified enhancement
- `//p` is a POTS enhancement
- `//s` is a socket enhancement
- `//x` is something to be removed
- `//*` is something to be fixed or implemented as soon as possible
