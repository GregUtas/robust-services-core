# Robust Services Core: Static Analysis C++11 Exclusions

The _ct_ directory contains a [parser](/ct/Parser.h) that supports the C++
static analysis tools. Because these tools were developed to analyze RSC,
the parser only supports the C++ language features that RSC uses. In fact,
RSC's source code is currently the only test suite for the `>parse` command.

Before RSC can use anything that `>parse` does not support, the parser
must be enhanced so that analyzing RSC's code is still possible. However,
`>parse` should also be enhanced to support things that RSC does not use,
so that other projects can also use the static analysis tools.
To this end, you are welcome to request that missing language features be
supported, and you are even _more_ than welcome to implement them.

Enhancing the parser to support a language feature is not enough. It would
be more accurate to say that `>parse` actually _compiles_ the code; there is much
that happens outside _Parser.h_. The `>parse` command
actually has an option that causes it to emit pseudo-code for a stack machine,
which is useful for checking whether the code was properly understood. Many
static analysis capabilities require this level of understanding, and `>parse`
even gathers information that a regular compiler would not.

### Recently Implemented
- [x] keywords `asm`, `alignas`, `alignof`, `goto`, `static_assert`, `thread_local`, `volatile`
- [x] `#pragma once` as alternative to `#include` guard
- [x] `using` for type aliases (as an alternative to `typedef`)
- [x] flexible order for keyword tags (e.g. `static`) used in function and data declarations/definitions
- [x] character escape sequences (`\`, `u8`, `u`, `U`)
- [x] prefixes for character and string literals (`u8`, `u`, `U`, `L`)
- [x] `= delete` and `= default` for constructor, destructor, and assignment operator
- [x] `"`_\<substr1>_`"`_\<whitespace>_`"`_\<substr2>_`"` as the continuation of a string literal

## Not Supported
The following is a list of things (through C++11) that are known _not_ to be
supported. In some cases, functions that would need to be enhanced to support
them are noted.

### Character Sets
All source code is assumed to be of type `char`.  `char8_t`, `char16_t`,
`char32_t`, and `wchar_t` are supported via escape codes and literal
prefixes (`u8`, `u`, `U`, `L`) but significant changes would be needed to
support them in identifiers, such as replacing uses of `std::string` in
the parser and other `CodeTools` classes.

### Reserved Words
- [ ] `decltype`
- [ ] `export` (removed in C\++11; reintroduced in C\++20)
- [ ] `register` (removed in C\++17)
- [ ] `and`, `and_eq`, `bitand`, `bitor`, `compl`, `not`, `not_eq`, `or`, `or_eq`, `xor`, `xor_eq`

### Preprocessor
- [ ] `#define` for any value other than an empty string or an integer literal
- [ ] `#if`: the conditional that follows the directive is ignored
- [ ] `#elif`: the conditional that follows the directive is ignored
- [ ] `#pragma`: parsed, but only `#pragma once` has any effect
- [ ] `#undef`: parsed but has no effect

  The conditional that follows `#if` or `#elif` is ignored because the evaluation
  of expressions that yield a constant has not been implemented. This capability
  would also be useful for other purposes.
  
  `#undef` could be supported but, given that all files are compiled together,
  would require checking as to whether it appeared in the transitive `#include`
  of the file currently being compiled.

There are no plans to support the following, which are odious and which would
force the introduction of a true preprocessing phase:

- [ ] `#` operator (to define a string literal)
- [ ] `##` operator (concatenation)
- [ ] function macros
- [ ] code aliases (_\<identifier> \<code>_)

The _only_ preprocessing that currently occurs before parsing is to
- analyze `#include` relationships to calculate a global parse order
- find and erase any macro name that is defined as an empty string

RSC's use of the preprocessor is restricted to
- `#include` directives
- `#ifndef` for `#include` guards
- `#ifdef` to include software that supports the target platform
- `#define` for a pseudo-keyword that maps to an empty string (`NO_OP` is the only current example)
- `#define` for a few integral constants in the _subs_ directory

  A constant of this type is effectively treated as if it had been declared
  using `constexpr`.

### Identifiers
- [ ] elaborated type specifiers (`class`, `struct`, `union`, or `enum` prefixed
to an identifier to act as an inline forward declaration or to resolve an ambiguity
caused by overloading an identifier)
- [ ] unnecessary name qualification

  Declaring a function as `Class::Function` causes the the parser to fail because
  this is one situation in which it expects an unqualified name.

### Character and String Literals
- [ ] raw string literals (`R` prefix)
- [ ] multi-character literals (e.g. `'AB'`)
- [ ] user-defined literals

See `Parser.GetCxxExpr`, `Parser.GetCxxAlpha`, `Parser.GetChar`, and `Parser.GetStr`.

### Declarations and Definitions
- [ ] identical declarations of data or functions

  Note that `Forward` supports multiple declarations of a class.
- [ ] identical definitions of anything, _even in separate translation units_

  All of the code is compiled together after calculating a global compile order,
  which makes the One Definition Rule global.

### Operators
- [ ] `operator.` chaining

  In `TlvMessage.DeleteParm`, `parm` is incorrectly flagged as `ArgumentCouldBeConst`.
  This occurs even though `parm.header.pid` is the target of an assignment. `StackArg` has
  a single `via_` member, so it can’t follow a _chain_ of `.` operators. It knows that `header`
  is modified, but it has dropped this information for `parm`.
- [ ] `operator?` second expression (the one after the `:`)

  `StackArg.via_` is incorrectly flagged as `DataCouldBeConst`. In `StackArg.SetNonConst`,
  this is caused by
  ```
  auto token = (index == 0 ? item : via_);
  ```
  Here, `token` is a _non_-const `CxxToken*`, courtesy of `item`.  If `via_` were assigned to
  the non-const `token`, we would know that `via_` could not be const, but this does not
  occur. The reason is that the _first_ expression afer the `?` operator is evaluated when
  executing the assignment operator, but not the second.

### Namespaces
- [ ] `using` statements in namespaces (currently treated as if at file scope)
- [ ] namespace aliases (with `using`)
- [ ] unnamed namespaces
- [ ] inline namespaces

See `Parser.GetNamespace`. Supporting any of these would also affect symbol resolution.

### Classes
- [ ] multiple inheritance (`Parser.GetBaseDecl`)
- [ ] tagging a base class as virtual (`Parser.GetBaseDecl`)
- [ ] non-public base class (allowed by parser, but accessibility checking does not enforce it)
- [ ] anonymous structs (`Parser.GetClassDecl`)
- [ ] an `enum`, `typedef`, or function in an anonymous union (allowed by parser, but
`CxxArea.FindEnum`, `CxxArea.FindFunc`, and `CxxArea.FindType` do not look for them)
- [ ] including a union instance immediately after defining it (`Parser.GetClassDecl`)
- [ ] pointer-to-member (the type _\<class>_`::*` and operators `.*` and `->*`)
- [ ] argument-dependent lookup in a class's scope
  - `getline` requires a `std::` prefix to be resolved but should find the version in `<string>`
  - `next` shouldn’t need a `std::` prefix, because `iterator` is already in `std`

### Functions
- [ ] function matching based on `volatile` (only `const` affects matching)
- [ ] member function suffix tags:
  - `const&`: equivalent to `const`
  - `&`: `this` must be non-const
  - `&&`: `this` must be an rvalue
- [ ] `noexcept(`_\<expr>_`)` as a function tag (only a bare `noexcept` is supported)
- [ ] using a different type (an alias) for an argument in the definition of a
previously declared function (`DataSpec.MatchesExactly`)
- [ ] argument-dependent lookup of regular functions (only done for operator
overloads)
- [ ] constructor inheritance (`Parser.GetUsing`, `Class.FindCtor`, and others)
- [ ] defining a class or function within a function (`Parser.ParseInBlock` and others)
- [ ] range-based `for` loops (`Parser.GetFor`, `Parser.GetTypeSpec`, and `Operation.Execute`)
- [ ] overloading the function call or comma operator

  The parser allows this, but calls to the overload won't be registered because
  `Operation.Execute` doesn't look for it.
- [ ] variadic argument lists
- [ ] lambdas (`Parser.GetArgument` and many others)
- [ ] dynamic exception specifications
- [ ] deduced return type (`auto`)
- [ ] trailing return type (after `->`)
- [ ] lvalues and rvalues

  `StackArg` does not distinguish lvalues and rvalues. Consequently,
  `Function.CanInvokeWith` cannot distinguish functions that are identical
  apart from the use of _\<argument-type>_`&` and _\<argument-type>_`&&`. Functions
  with the latter signature will therefore be logged as `FunctionIsUnused`
  by `>check`.
- [ ] A function call on a constructor is not registered when brace initialization
is used. The constructor might therefore be logged as `FunctionIsUnused`
by `>check`.

### Data
- [ ] declaring more than one data instance in the same statement, either at file
scope or within a class (`Parser.GetClassData` and `Parser.GetSpaceData`)

  Note that `FuncData` supports this _within_ a function (for example, `int i = 0, *pi = nullptr`).
- [ ] type matching based on `volatile` (only `const` affects matching)
- [ ] unnamed bit fields (`Parser.GetClassData`)

### Enumerations
- [ ] accessing an enumeration or enumerator using `.` or `->` instead of `::`
- [ ] scoped enumerations (`enum class`, `enum struct`)

  Once this is supported, some enumerations at namespace scope should make use of it.
  However, converting from a scoped enumeration to an `int` requires a `static_cast`,
  so an enumeration that often acts as a `const int` should not be converted.
- [ ] opaque enumerations
- [ ] forward declarations of enumerations
- [ ] argument-dependent lookup in an enumeration's scope

### Typedefs
- [ ] `typedef enum` (`Parser.GetTypedef`)
- [ ] `typedef struct` (`Parser.GetTypedef`)

### Templates
- [ ] template arguments other than qualified names

  For example, `std::bitset<sizeof(uint8_t)>` would have to be written as
  `std::bitset<bytesize>` following `constexpr size_t bytesize = sizeof(uint8_t)`.
- [ ] a constructor call that requires template argument deduction when a
template is a base class

  The template argument must currently be included after the class template name.
- [ ] instantiation of the entire class template occurs when a member is used

  The definition of the template argument(s) must be visible
  at this point, even if they are not needed for a successful compile (e.g.
  if the template's code only uses the type `T*`, not `T`)
- [ ] explicit instantiation
- [ ] `using` for alias templates (`Parser.GetUsing` and others)
- [ ] `extern template`
- [ ] initialization of static data members in template instances

  The code for this is not included when generating the code for a template instance,
  so the static member appears uninitialized (`<@i=0`) in the _.lib_ file created by
  `>export`.

Because external headers in the [_subs_](/subs) directory do not provide function
implementations for templates, `>check` erroneously recommends things such as
- removing an `#include` that is needed to make a destructor visible to a
`std::unique_ptr` template instance
- declaring a data member `const`even though it is inserted in a `std::set` and
must therefore support `std::move`
- removing most of the things in _Allocators.h_ (since this code is only invoked
from the STL, not from within RSC)

### Parser
- [ ] `Parser.Punt` causes a software log on argument overflow
- [ ] `Scope` and `clear` are repeated in pseudo-code generated when `>parse`
is used with the `x` option
