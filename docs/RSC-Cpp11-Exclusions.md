# Robust Services Core: Static Analysis C++11 Exclusions

The _ct_ directory contains a [parser](/src/ct/Parser.h) that supports the C++
static analysis tools. Because these tools were developed to analyze RSC, the
parser only supports some C++ features that RSC does not use.  In fact, RSC's
source code is the only test suite for the `>parse` command.

Except for C\++17's `<filesystem>` and `[[fallthrough]]`, RSC uses a subset
of C\++11. This document therefore describes features, through C\++11,
that the parser does not support. You can also assume that more recent C++
features are not supported, at least if they involve new syntax.

Before RSC can use anything that `>parse` does not support, the parser
must be enhanced so that analyzing RSC's code is still possible. However,
`>parse` should also be enhanced to support things that RSC does _not_ use,
so that the projects that use them can also use the static analysis tools.
To this end, you are welcome to request that missing language features be
supported, and you are even _more_ than welcome to implement them.

Enhancing the parser to support a language feature is not enough. It would
be more accurate to say that `>parse` actually _compiles_ the code; there is
a lot that happens outside _Parser.cpp_. The `>parse` command
actually has an option that causes it to emit pseudo-code for a stack machine,
which is useful for checking whether the code was properly understood. Many
static analysis capabilities require this level of understanding, and `>parse`
even gathers information that a regular compiler would not.

### Recently Implemented
- [x] elaborated type specifiers (`class`, `struct`, `union`, or `enum`
  prefixed to an identifier to resolve an ambiguity caused by overloading
  a symbol)
- [x] `[[fallthrough]];`
- [x] constant expressions as template arguments
- [x] brace initialization of members in a constructor's initialization list
- [x] keywords `asm`, `alignas`, `alignof`, `goto`, `static_assert`,
  `thread_local`, `volatile`
- [x] `#pragma once` as alternative to `#include` guard
- [x] `using` for type aliases (as an alternative to `typedef`)
- [x] flexible order for keyword tags (e.g. `static`) used in function and
  data declarations/definitions
- [x] character escape sequences (`\`, `u8`, `u`, `U`)
- [x] prefixes for character and string literals (`u8`, `u`, `U`, `L`)
- [x] `= delete` and `= default` for constructor, destructor, and assignment
  operator
- [x] `"`_\<substr1>_`"`_\<whitespace>_`"`_\<substr2>_`"` as the continuation
  of a string literal

## Not Supported
The following is a list of things (through C++11) that are known to _not_ be
supported.

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
- [ ] `and`, `and_eq`, `bitand`, `bitor`, `compl`, `not`, `not_eq`, `or`,
  `or_eq`, `xor`, `xor_eq`

### Preprocessor
- [ ] `#define` for any value other than an empty string or an integer literal
- [ ] `#if`: the conditional that follows the directive is ignored
- [ ] `#elif`: the conditional that follows the directive is ignored
- [ ] `#pragma`: parsed, but only `#pragma once` has any effect
- [ ] `#undef`: parsed but has no effect

  The conditional that follows `#if` or `#elif` is ignored because folding (the
  evaluation of expressions that yield a constant) has not been implemented. This
  capability would also be useful for other purposes.
  
  `#undef` could be supported but, given that all files are compiled together,
  would require checking as to whether it appeared in the transitive `#include`
  of the file currently being compiled. In any case, it's a total hack.

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
- `#define` for a pseudo-keyword that maps to an empty string (`NO_OP` and
  `NO_FT` are the only current examples)
- `#define` for a few integral constants in the _subs_ directory

  A constant of this type is effectively treated as if it had been declared
  using `constexpr`.

### Identifiers
- [ ] elaborated type specifiers cannot act as forward declarations
- [ ] unnecessary name qualification

  Declaring a function as `Class::Function` causes the the parser to fail
  because this is one situation in which it expects an unqualified name.

### Character and String Literals
- [ ] raw string literals (`R` prefix)
- [ ] multi-character literals (e.g. `'AB'`)
- [ ] user-defined literals

### Declarations and Definitions
- [ ] identical declarations of data or functions

  Note that `Forward` supports multiple declarations of a class.
- [ ] identical definitions of anything, _even in separate translation units_

  All of the code is compiled together after calculating a global compile order,
  which makes the One Definition Rule global.

### Namespaces
- [ ] `using` statements in namespaces (currently treated as if at file scope)
- [ ] namespace aliases (with `using`)
- [ ] unnamed namespaces
- [ ] inline namespaces

Supporting any of these would also affect symbol resolution. An unnamed namespace
can be removed by defining data and functions that appear within it as `static`.

### Classes
- [ ] multiple inheritance
- [ ] a base class that is `virtual`
- [ ] a base class that is `private` or `protected`

  The parser allows this, but accessibility checking does not enforce it.
- [ ] local classes (defining a class within a function)
- [ ] anonymous structs
- [ ] an enum, typedef, or function in an anonymous union

  The parser allows this, but symbol lookup functions do not search for them.
- [ ] including a class/struct/union/enum instance immediately before the
  semicolon at the end of its definition
- [ ] pointer-to-membe (the type _\<class>_`::*` and operators `.*` and `->*`)

### Functions
- [ ] function matching based on `volatile` (only `const` affects matching)
- [ ] member function suffix tags:
  - `const&`: equivalent to `const`
  - `&`: `this` must be non-const
  - `&&`: `this` must be an rvalue
- [ ] `noexcept(`_\<expr>_`)` as a function tag (only a bare `noexcept` is
  supported)
- [ ] using a different type (an alias) for an argument in the definition of a
  previously declared function
- [ ] argument-dependent lookup of regular functions (only done for operator
  overloads)
  - `getline` requires a `std::` prefix to be resolved but should find the
     version in `<string>`
  - `next` shouldn’t need a `std::` prefix (`iterator` is in `std`)
  - `end` shouldn't need a `std::filesystem::` prefix (`directory_iterator`
    is in `std::filesystem`)
- [ ] constructor inheritance
- [ ] range-based `for` loops
- [ ] overloading the function call or comma operator

  The parser allows this, but calls to the overload won't be registered because
  overloads of these operators are not searched for.
- [ ] variadic argument lists
- [ ] lambdas
- [ ] dynamic exception specifications
- [ ] deduced return type (`auto`)
- [ ] trailing return type (after `->`)

### Data
- [ ] declaring more than one data instance in the same statement, either at file
  scope or within a class

  Note that this is supported _within_ a function (for example,
  `int i = 0, *pi = nullptr`).
- [ ] type matching based on `volatile` (only `const` affects matching)
- [ ] unnamed bit fields

### Enumerations
- [ ] accessing an enumeration or enumerator using `.` or `->` instead of `::`
- [ ] scoped enumerations (`enum class`, `enum struct`)

  Once this is supported, some enumerations at namespace scope should make use
  of it. However, converting from a scoped enumeration to an `int` requires a
  `static_cast`, so an enumeration that often acts as a `const int` should not
  be converted.
- [ ] opaque enumerations
- [ ] forward declarations of enumerations
- [ ] argument-dependent lookup in an enumeration's scope

### Typedefs
- [ ] `typedef enum` (common in pure C)
- [ ] `typedef struct` (common in pure C)

### Templates
- [ ] a constructor call that requires template argument deduction when a
  template is a base class

  The template argument must currently be included after the class template name.
- [ ] instantiation of the entire class template occurs when a member is used

  The definition of the template argument(s) must be visible
  at this point, even if they are not needed for a successful compile (e.g.
  if the template's code only uses the type `T*`, not `T`)
- [ ] template argument deduction for function templates

  This is currently somewhat weak. If more than one function template has the
  same name and the wrong template gets selected, two workarounds are to (a)
  adopt unique names or (b) specify the template argument explicitly.
- [ ] explicit instantiation
- [ ] `using` for alias templates
- [ ] `extern template`
- [ ] `std::list::sort`: The _subs_ file for `list` declares the sort function
  as `bool (*sorted)(T& first, T& second)`. Therefore, if its parameters
  differ from the `list` template arguments, a log occurs during function
  matching. For example, the log occurs if the sort function uses a base class
  of `T` rather than `T` itself.

## False Positives and Negatives from `>check`

The `>check` command sometimes produces erroneous warnings. Some of them occur
because of how the `StackArg` class tracks the use of variables in executable
code.

- [ ] `IncludeRemove`

  Because external headers in the [_subs_](/src/subs) directory
  do not provide function implementations for templates, `>check` can incorrectly
  suggest removing an `#include` that is needed to make a destructor visible to
  a `std::unique_ptr` template instance.
  
- [ ] `UsingRemove`

  _SbIpBuffer.cpp_ is told to remove `using NetworkBase`. However, it is needed
  for the `IpBuffer*` returned by `SbIpBuffer.Clone`. The logs occurs because the
  return type is considered to be in a function's scope, whereas the scope should
  not begin until the function's arguments.

- [ ] `ArgumentCouldBeConst` and `DataCouldBeConst`

  - **`operator.` chaining**.  In `TlvMessage.DeleteParm`, `parm` is
  incorrectly flagged as `ArgumentCouldBeConst`. This occurs even though
  `parm.header.pid` is the target of an assignment. `StackArg` has a single
  `via_` member, so it can’t follow a _chain_ of `.` operators. It knows that
  `header` is modified, but it has dropped this information for `parm`.

  - **`operator?` "else" expression**. `StackArg.via_` is incorrectly flagged
  as `DataCouldBeConst`. In `StackArg.SetNonConst`, this is caused by
    ```
    auto token = (index == 0 ? item : via_);
    ```
    Here, `token` is a _non_-const `CxxToken*` (the type imputed from `item`).
    If `via_` were assigned to the non-const `token`, we would know that
   `via_` could not be const, but this does not occur. The reason is that the
    _first_ expression afer the `?` operator is evaluated when executing the
    assignment operator, but not the second.

  - **Templates**. Because external headers in the [_subs_](/src/subs) directory
  do not provide function implementations for templates, `>check` can incorrectly
  suggest declaring a data member `const` even though it is inserted in a
  `std::set` and must therefore support `std::move`.

  - **Overloaded functions**. If an item is not declared `const` and it invokes
  an overloaded function that returns either a const or non-const result (such
  as `vector::at`), the item is not flagged as "could be const" even though
  it could be. This occurs because the non-const version of the overloaded
  function is selected when resolving the function call for a non-const item,
  effectively making its non-const declaration self-fulfilling.

- [ ] `FunctionIsUnused`

  - **lvalues and rvalues**. `StackArg` does not distinguish these.
  Consequently, `Function.CanInvokeWith` cannot choose between two functions
  that are identical except for the use of _\<argument-type>_`&` and
  _\<argument-type>_`&&`. Functions with the latter signature are therefore
  flagged as unused.

  - **Brace initialization**. A function call on a constructor is not recorded
  when brace initialization is used. The constructor is therefore flagged as
  unused unless explicitly invoked elsewhere.

  - **Only invoked through base**. If a derived class implements `operator
  delete` but is only deleted through a pointer to its base class, its version
  of `operator delete` will be flagged as unused.
  
- [ ] `FunctionCouldBeFree`

  This is logged on `MapAndUnits::delete_clone`. However, it invokes a private
  destructor, so it cannot be a free function without also being a friend of
  the class that it deletes. The log occurs because the destructor isn't the
  referent of either component in the expression `delete clone`: the referent
  of `delete` is the appropriate `operator delete`, and the referent of `clone`
  is the function's argument. Somehow, the destructor also needs to be recorded
  as a usage. An invocation is registered on it, but there is currently no item
  against which to record its usage. Effectively, the problem is that `delete`
  invokes _two_ functions.

- [ ] `RedundantScope`

  A redundant scope warning is not generated when a
  scope is prefixed to a symbol that is also declared in an outer namespace or
  outer class of the current scope. But if the symbol is ambiguous only because
  of a `using` statement, the warning will be incorrectly generated.
