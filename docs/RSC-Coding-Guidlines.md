# Robust Services Core: Coding Guidelines

## Formatting
1. Begin each file with the following heading:
```
  //================================================================================
  //
  //  <FileName>
  //
  //  Copyright (C) 201n-201n <Name>.  All rights reserved.
  //
```
2. Use spaces instead of tabs.
1. Indent a multiple of 3 spaces.
1. Remove unnecessary interior spaces and semicolons.  Remove trailing spaces.
1. Use `//` comments instead of `/*...*/`.
1. Add blank lines for readability, but avoid multiple blank lines.
1. Limit lines to 80 characters in length.  Break after `,:)` and before `(`.  A break at an operator can either occur before or after, depending on which reads better.
1. Almost always use Camel case.  Use all uppercase and underscores only in low-level types and constants.  Names that evoke Hungarian notation are an abomination.
1. Keep `*` and `&` with the type instead of the variable (`Type* t` instead of `Type *t`).

## Interfaces
1. Insert an `#include` guard based on the file name (`FileName.ext` and `FILENAME_EXT_INCLUDED`) immediately after the standard heading.1. Sort `#include` statements as follows:
   1. the header that defines the base class of the class defined in the header
   1. C++/C library headers, in alphabetical order
   1. other headers, in alphabetical order
1. Remove an `#include` solely associated with functions inherited from a base class.
1. Remove an `#include` by forward-declaring a class that is only named in references or pointers.  Use an explicit forward declaration instead of relying on this as a side effect of a friend declaration.
1. Remove using declarations and directives.  Prefix the namespace directly (i.e. `std::`\<symbol>).
1. Initialize global data (static members) in the .cpp if possible.

## Implementations
1. Order `#include` statements as follows:
   1. the header that defines the functions being implemented
   1. C++/C library headers, in alphabetical order
   1. other headers, in alphabetical order
1. Omit any `#include` or using that is already in the header.
1. Put all of the code in the same namespace as the class defined in the header.
1. Implement functions in alphabetical order, following the constructor(s) and destructor.
1. Separate functions in the same class with a `//----`... that is 80 characters long.
1. Separate private classes (local to the .cpp) with a `//====`... that is 80 characters long.
1. Add a blank line after the `fn_name` that defines a function’s name for `Debug::ft`.
1. Name constructors and destructors `"<class>.ctor"` and `"<class>.dtor"` for `Debug::ft`.
1. Fix compiler warnings through level 4.

## Classes
1. Give a class its own .h and .cpp unless it is trivial, closely related to others, or private to an implementation.
1. A base class should be abstract.  Its constructor should therefore be protected.
1. Tag a constructor as `explicit` if it can be invoked with one argument.
1. Make each public function non-virtual, with a one-line invocation of a virtual function if necessary.
1. Make each virtual function private if possible, or protected if derived classes may need to invoke it.
1. Make a base class destructor
   1. virtual and public
   1. non-virtual and protected
   1. virtual and protected, to restrict deletion
1. If a destructor frees a resource, even automatically through a `unique_ptr` member, also define
   1. a copy constructor: `Class(const Class& that);`
   1. a copy assignment operator: `Class& operator=(const Class& that);`
If the class allows copying, also define
   3. a move constructor: `Class(Class&& that);`
   1. a move assignment operator: `Class& operator=(Class&& that);`
   - In C++11, each of the above functions can be suffixed with `= delete` to prohibit its use, or `= default` to use the compiler-generated default.  The pre-C++11 equivalents are to make the function private (`delete`) or not declare it at all (`default`).
   - In a copy assignment operator, create copies of `that`’s resources first, then release `this`’s existing resources, and finally assign the new ones.
   - A "move" function is an optimization that releases the existing resources and then takes over the ones owned by that.  Its implementation typically uses `std::swap`.
1. To prohibit stack allocation, make constructors private, and/or make the destructor private.
1. To prohibit scalar heap allocation, define `operator new` as private.
1. To prohibit vector heap allocation, define `operator new[]` as private.
1. If a class only has static members, convert it to a namespace.  If this is not possible, prohibit its creation.
1. Include `virtual` and `override` when overriding a function defined in a base class.
1. Make a function or argument const when appropriate.
1. Remove `inline` as a keyword.
1. Avoid `friend` where possible.
1. Override `Display` if a class has data.
1. Override `Patch` except in a trivial leaf class.
1. Avoid invoking virtual functions in the same class hierarchy within constructors and destructors.
1. Provide an implementation for a pure virtual function to highlight the bug of calling it too early during construction or too late during destruction.
1. If a class is large, consider using the PIMPL idiom to move its private members to the .cpp.
1. When only a subset of a class’s data should be write-protected, split it into a pair of collaborating classes that use `MemProt` and `MemDyn` (or `MemImm` and `MemPerm`).
1. Static member data begins with an uppercase letter and ends with an underscore, which may be omitted if it is not returned by a "Get" function.  Non-static member data begins with a lowercase letter and ends with an underscore, which may be omitted if it is public (in which case it should probably be in a struct).

## Functions
1. Use the initialization list for constructors.  Initialize members in the order that the class declared them.
1. Use `()` instead of `(void)` for an empty argument list.
1. Name each argument.  Use the same name in the interface and the implementation.
1. Make the invocation of `Debug::ft` the first line in a function body, and follow it with a blank line.
1. The `fn_name` passed to `Debug::ft` and similar functions should accurately reflect the name of the invoking function.
1. A simple "Get" function should not invoke `Debug::ft` unless it is virtual.
1. Left-align the types and variable names in a long declaration list.
1. Use `nullptr` instead of `NULL`.
1. Check for `nullptr`, even when an argument is passed by reference. (A reference merely documents that `nullptr` is an invalid input.)
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
