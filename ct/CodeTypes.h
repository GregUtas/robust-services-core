//==============================================================================
//
//  CodeTypes.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
//
#ifndef CODETYPES_H_INCLUDED
#define CODETYPES_H_INCLUDED

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <set>
#include <string>
#include <vector>
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
//  For lists of strings.
//
typedef std::vector< std::string > stringVector;

//  C++ keyword strings.
//
extern NodeBase::fixed_string ALIGNAS_STR;
extern NodeBase::fixed_string ALIGNOF_STR;
extern NodeBase::fixed_string ASM_STR;
extern NodeBase::fixed_string AUTO_STR;
extern NodeBase::fixed_string BOOL_STR;
extern NodeBase::fixed_string BREAK_STR;
extern NodeBase::fixed_string CATCH_STR;
extern NodeBase::fixed_string CASE_STR;
extern NodeBase::fixed_string CHAR_STR;
extern NodeBase::fixed_string CHAR16_STR;
extern NodeBase::fixed_string CHAR32_STR;
extern NodeBase::fixed_string CLASS_STR;
extern NodeBase::fixed_string CONST_STR;
extern NodeBase::fixed_string CONST_CAST_STR;
extern NodeBase::fixed_string CONSTEXPR_STR;
extern NodeBase::fixed_string CONTINUE_STR;
extern NodeBase::fixed_string DEFAULT_STR;
extern NodeBase::fixed_string DELETE_STR;
extern NodeBase::fixed_string DELETE_ARRAY_STR;
extern NodeBase::fixed_string DOUBLE_STR;
extern NodeBase::fixed_string DYNAMIC_CAST_STR;
extern NodeBase::fixed_string DO_STR;
extern NodeBase::fixed_string ELSE_STR;
extern NodeBase::fixed_string ENUM_STR;
extern NodeBase::fixed_string EXPLICIT_STR;
extern NodeBase::fixed_string EXTERN_STR;
extern NodeBase::fixed_string FALLTHROUGH_STR;
extern NodeBase::fixed_string FALSE_STR;
extern NodeBase::fixed_string FINAL_STR;
extern NodeBase::fixed_string FLOAT_STR;
extern NodeBase::fixed_string FOR_STR;
extern NodeBase::fixed_string FRIEND_STR;
extern NodeBase::fixed_string GOTO_STR;
extern NodeBase::fixed_string IF_STR;
extern NodeBase::fixed_string INLINE_STR;
extern NodeBase::fixed_string INT_STR;
extern NodeBase::fixed_string LONG_STR;
extern NodeBase::fixed_string MUTABLE_STR;
extern NodeBase::fixed_string NAMESPACE_STR;
extern NodeBase::fixed_string NEW_STR;
extern NodeBase::fixed_string NEW_ARRAY_STR;
extern NodeBase::fixed_string NOEXCEPT_STR;
extern NodeBase::fixed_string NULLPTR_STR;
extern NodeBase::fixed_string NULLPTR_T_STR;
extern NodeBase::fixed_string OPERATOR_STR;
extern NodeBase::fixed_string OVERRIDE_STR;
extern NodeBase::fixed_string PRIVATE_STR;
extern NodeBase::fixed_string PROTECTED_STR;
extern NodeBase::fixed_string PUBLIC_STR;
extern NodeBase::fixed_string REINTERPRET_CAST_STR;
extern NodeBase::fixed_string RETURN_STR;
extern NodeBase::fixed_string SHORT_STR;
extern NodeBase::fixed_string SIGNED_STR;
extern NodeBase::fixed_string SIZEOF_STR;
extern NodeBase::fixed_string STATIC_STR;
extern NodeBase::fixed_string STATIC_ASSERT_STR;
extern NodeBase::fixed_string STATIC_CAST_STR;
extern NodeBase::fixed_string STRUCT_STR;
extern NodeBase::fixed_string SWITCH_STR;
extern NodeBase::fixed_string TEMPLATE_STR;
extern NodeBase::fixed_string THIS_STR;
extern NodeBase::fixed_string THREAD_LOCAL_STR;
extern NodeBase::fixed_string THROW_STR;
extern NodeBase::fixed_string TRUE_STR;
extern NodeBase::fixed_string TRY_STR;
extern NodeBase::fixed_string TYPEDEF_STR;
extern NodeBase::fixed_string TYPEID_STR;
extern NodeBase::fixed_string TYPENAME_STR;
extern NodeBase::fixed_string UNION_STR;
extern NodeBase::fixed_string UNSIGNED_STR;
extern NodeBase::fixed_string USING_STR;
extern NodeBase::fixed_string VIRTUAL_STR;
extern NodeBase::fixed_string VOID_STR;
extern NodeBase::fixed_string VOLATILE_STR;
extern NodeBase::fixed_string WCHAR_STR;
extern NodeBase::fixed_string WHILE_STR;

extern NodeBase::fixed_string DEFINED_STR;
extern NodeBase::fixed_string HASH_DEFINE_STR;
extern NodeBase::fixed_string HASH_ELIF_STR;
extern NodeBase::fixed_string HASH_ELSE_STR;
extern NodeBase::fixed_string HASH_ENDIF_STR;
extern NodeBase::fixed_string HASH_ERROR_STR;
extern NodeBase::fixed_string HASH_IF_STR;
extern NodeBase::fixed_string HASH_IFDEF_STR;
extern NodeBase::fixed_string HASH_IFNDEF_STR;
extern NodeBase::fixed_string HASH_INCLUDE_STR;
extern NodeBase::fixed_string HASH_LINE_STR;
extern NodeBase::fixed_string HASH_PRAGMA_STR;
extern NodeBase::fixed_string HASH_UNDEF_STR;

//------------------------------------------------------------------------------
//
//  Other parser strings.
//
extern NodeBase::fixed_string ARRAY_STR;
extern NodeBase::fixed_string COMMENT_BEGIN_STR;
extern NodeBase::fixed_string COMMENT_END_STR;
extern NodeBase::fixed_string COMMENT_STR;
extern NodeBase::fixed_string ELLIPSES_STR;
extern NodeBase::fixed_string LOCALS_STR;
extern NodeBase::fixed_string NULL_STR;

//------------------------------------------------------------------------------
//
//  Returns the indentation size for source code.
//
size_t IndentSize();

//  Returns the maximum line length for source code.
//
size_t LineLengthMax();

//------------------------------------------------------------------------------
//
//  Valid initial characters in an identifier.  '#' and '~' are included so that
//  preprocessor directives and destructor names can be treated as keywords and
//  identifiers, respectively.
//
extern const std::string ValidFirstChars;

//  Valid subsequent characters in an identifier.
//
extern const std::string ValidNextChars;

//  Valid subsequent characters in a template specification.
//
extern const std::string ValidTemplateSpecChars;

//  Valid characters in an operator.
//
extern const std::string ValidOpChars;

//  Valid characters in an integer literal.
//
extern const std::string ValidIntChars;

//  Valid digits in an integer, hex, and octal literal, respectively.
//
extern const std::string ValidIntDigits;
extern const std::string ValidHexDigits;
extern const std::string ValidOctDigits;

//  Whitespace characters.
//
extern const std::string WhitespaceChars;

//  Single (//------...) and double (///======...) rules.
//
extern const std::string SingleRule;
extern const std::string DoubleRule;

//------------------------------------------------------------------------------
//
//  How to sort items when displaying them.
//
enum ItemSort
{
   SortByName,    // sort by name (case ignored)
   SortByPos,     // sort by position within file (all items in same file)
   SortByFilePos  // sort by file, then position within file
};

//------------------------------------------------------------------------------
//
//  Restrictions when looking for a name (e.g. in a type or identifier).
//
enum Constraint
{
   NonKeyword,   // must not be a keyword
   TypeKeyword,  // may only be a type keyword (e.g. int)
   AnyKeyword    // may be a keyword
};

//------------------------------------------------------------------------------
//
//  For adding and removing levels of pointer indirection and for counting
//  arrays and references.
//
typedef int8_t TagCount;

//------------------------------------------------------------------------------
//
//  Types of functions.
//
enum FunctionType
{
   FuncCtor,
   FuncDtor,
   FuncOperator,
   FuncStandard
};

//------------------------------------------------------------------------------
//
//  Roles of functions.
//
enum FunctionRole
{
   PureCtor,   // constructor for a new object
   PureDtor,   // destructor
   CopyCtor,   // copy constructor
   MoveCtor,   // move constructor
   CopyOper,   // copy (assignment) operator
   MoveOper,   // move (assignment) operator
   FuncOther,  // none of those above
   FuncRole_N  // number of roles
};

//  Inserts a string for ROLE into STREAM.
//
std::ostream& operator<<(std::ostream& stream, FunctionRole role);

//------------------------------------------------------------------------------
//
//  How and where a function that can be defaulted or deleted is defined.
//
enum FunctionDefinition
{
   NotDeclared,    // not declared in this class or a base class
   LocalDeclared,  // declared by class (implemented or defaulted)
   LocalDeleted,   // deleted by this class
   BaseDefined,    // defined by base class (implemented or defaulted)
   BaseDeleted     // deleted by base class
};

//------------------------------------------------------------------------------
//
//  Controls whether a function is compiled (to produce pseudo object code)
//  and, for a header, indicates whether it contains function templates or an
//  entire class template.
//
enum TemplateType
{
   NonTemplate,   // not a template (or a function not in a template)
   FuncTemplate,  // a function template, which could be in a class template
   ClassTemplate  // a class template (or a function in a class template)
};

//------------------------------------------------------------------------------
//
//  The distance between a class and subclass or a scope and subscope.
//  NOT_A_SUBSCOPE and NOT_A_SUBCLASS indicate that the distance is
//  "infinite".
//
typedef uint8_t Distance;

constexpr Distance NOT_A_SUBSCOPE = UINT8_MAX;
constexpr Distance NOT_A_SUBCLASS = UINT8_MAX;

//------------------------------------------------------------------------------
//
//  The accessibility of a symbol, based on the scope that declares it and
//  the scope that uses it.  Whether the symbol is visible (through #include
//  and using statements, and scope qualification) is considered separately.
//
enum Accessibility
{
   Declared,      // user is declarer
   Inherited,     // user is a subclass of declarer and can see name
   Unrestricted,  // defined in a .h* or user is a friend of declarer
   Restricted,    // defined in a .c*
   Inaccessible   // none of the above apply (e.g. private)
};

//------------------------------------------------------------------------------
//
//  Indicates how closely a type matches the one expected by a function or
//  template.
//
enum TypeMatch
{
   Incompatible,   // argument cannot be matched
   Adaptable,      // argument is non-const but would be passed as const
   Abridgeable,    // argument could be truncated (e.g. int to char)
   Convertible,    // argument must be converted (e.g. enum to int)
   Constructible,  // argument must be passed to a constructor
   Promotable,     // argument can be promoted (numeric to one of greater range)
   Derivable,      // argument is derived from expected argument
   Compatible      // argument matches without modification
};

//------------------------------------------------------------------------------
//
//  Where a TypeSpec occurs.
//
enum TypeSpecUser
{
   TS_Unspecified,  // default value
   TS_Definition,   // data or function definition, distinct from declaration
   TS_Function,     // function declaration
};

//------------------------------------------------------------------------------
//
//  Specifies a type's role in a template.
//
enum TemplateRole
{
   TemplateNone,       // not part of a template
   TemplateArgument,   // e.g. int in vector< int >
   TemplateParameter,  // e.g. T in template< typename T > class vector {...};
   TemplateClass       // a DataSpec created internally for template matching;
                       // contains each of the parameters to a class template
};

//------------------------------------------------------------------------------
//
//  Types of assignments.
//
enum AssignmentType
{
   Copied,   // right-hand side of an assignment operator
   Passed,   // passed as an argument
   Returned  // returned as a function result
};

//------------------------------------------------------------------------------
//
//  Groups for sorting #include directives.  An "external file" is one whose
//  name is enclosed in angle brackets rather than quotes.
//
enum IncludeGroup
{
   ExtDecl,   // external file declaring an item defined in this file
   IntDecl,   // internal file declaring an item defined in this file
   ExtBase,   // external file defining a base class of a class in this file
   IntBase,   // internal file defining a base class of a class in this file
   ExtUses,   // external file declaring an item used in this file
   IntUses,   // internal file declaring an item used in this file
   Ungrouped  // group not determined
};

//------------------------------------------------------------------------------
//
//  What type of function is updating the cross-reference.
//
enum XrefUpdater
{
   NotAFunction,      // being updated by something besides a function
   StandardFunction,  // a regular function
   TemplateFunction,  // a function in a template
   InstanceFunction   // a function in a template instance
};

//------------------------------------------------------------------------------
//
//  Source code warnings.
//
enum Warning
{
   AllWarnings,              // used as a wildcard when fixing warnings
   UseOfNull,                // use of NULL
   PtrTagDetached,           // <type> *<data> instead of <type>* data
   RefTagDetached,           // <type> &<data> instead of <type>& data
   UseOfCast,                // use of C-style cast: (type) expr
   FunctionalCast,           // type(expr) is equivalent to a C-style cast
   ReinterpretCast,          // use of reinterpret_cast
   Downcasting,              // use of cast down inheritance hierarchy
   CastingAwayConstness,     // use of cast to remove const qualifier
   PointerArithmetic,        // use of pointer arithmetic
   RedundantSemicolon,       // unnecessary semicolon
   RedundantConst,           // more than one const qualifier for same token
   DefineNotAtFileScope,     // #define appears within a class or function
   IncludeFollowsCode,       // #include appears after code
   IncludeGuardMissing,      // no #include guard
   IncludeNotSorted,         // #include not sorted in standard order
   IncludeDuplicated,        // #include already exists for this file
   IncludeAdd,               // #include should be added
   IncludeRemove,            // #include should be removed
   RemoveOverrideTag,        // function is also tagged final
   UsingInHeader,            // header contains using directive or declaration
   UsingDuplicated,          // using statement duplicated
   UsingAdd,                 // using statement should be added
   UsingRemove,              // using statement should be removed
   ForwardAdd,               // forward declaration should be added
   ForwardRemove,            // forward declaration should be removed
   ArgumentUnused,           // argument not used
   ClassUnused,              // no members used
   DataUnused,               // data is neither read nor written
   EnumUnused,               // enum not referenced
   EnumeratorUnused,         // enumerator not referenced
   FriendUnused,             // friend did not access a restricted member
   FunctionUnused,           // function not invoked
   TypedefUnused,            // typedef not referenced
   ForwardUnresolved,        // no referent found for forward declaration
   FriendUnresolved,         // no referent found for friend declaration
   FriendAsForward,          // friend declaration is also forward declaration
   HidesInheritedName,       // member has the same name as a base class member
   ClassCouldBeNamespace,    // only has enums, typedefs, and static functions
   ClassCouldBeStruct,       // has no private members or subclasses
   StructCouldBeClass,       // has private members
   RedundantAccessControl,   // previous member already public/protected/private
   ItemCouldBePrivate,       // item only used within declarer
   ItemCouldBeProtected,     // item only used within declarer and subclasses
   PointerTypedef,           // typedef of pointer type
   AnonymousEnum,            // declaration of unnamed enum
   DataUninitialized,        // global data is not initialized
   DataInitOnly,             // data is initialized but not read or written
   DataWriteOnly,            // data is not read
   GlobalStaticData,         // global static data defined in header
   DataNotPrivate,           // data member is not private
   DataCannotBeConst,        // for detecting const logic errors
   DataCannotBeConstPtr,     // for detecting const logic errors
   DataCouldBeConst,         // data item could be declared as const
   DataCouldBeConstPtr,      // data item could be declared as a const pointer
   DataNeedNotBeMutable,     // no const function modifies this data member
   ImplicitPODConstructor,   // use of implicit constructor; has POD member
   ImplicitConstructor,      // use of implicit constructor
   ImplicitCopyConstructor,  // use of implicit copy constructor
   ImplicitCopyOperator,     // use of implicit copy (assignment) operator
   PublicConstructor,        // base class has public constructor
   NonExplicitConstructor,   // constructor should be tagged explicit
   MemberInitMissing,        // item missing from member initialization list
   MemberInitNotSorted,      // item missorted in member initialization list
   ImplicitDestructor,       // use of implicit destructor
   VirtualDestructor,        // virtual destructor is not public
   NonVirtualDestructor,     // non-virtual base class destructor not protected
   VirtualFunctionInvoked,   // constructor or destructor calls virtual function
   RuleOf3DtorNoCopyCtor,    // destructor defined, but not copy constructor
   RuleOf3DtorNoCopyOper,    // destructor defined, but not copy operator
   RuleOf3CopyCtorNoOper,    // copy constructor defined, but not copy operator
   RuleOf3CopyOperNoCtor,    // copy operator defined, but not copy constructor
   OperatorOverloaded,       // operator && or || overloaded
   FunctionNotDefined,       // function not implemented
   PureVirtualNotDefined,    // pure virtual function not implemented
   VirtualAndPublic,         // function is both public and virtual
   BoolMixedWithNumeric,     // bool used as numeric or vice versa
   FunctionNotOverridden,    // virtual function has no overrides
   RemoveVirtualTag,         // function is an override or is tagged final
   OverrideTagMissing,       // add override tag to function declaration
   VoidAsArgument,           // use of (void) to specify function parameter
   AnonymousArgument,        // declaration of unnamed argument
   AdjacentArgumentTypes,    // adjacent arguments have the same type
   DefinitionRenamesArgument, // names in declaration and definition differ
   OverrideRenamesArgument,  // names in override and root base class differ
   VirtualDefaultArgument,   // virtual function defines default argument
   ArgumentCannotBeConst,    // for detecting const logic errors
   ArgumentCouldBeConstRef,  // object could be passed by const reference
   ArgumentCouldBeConst,     // argument could be declared as const
   FunctionCannotBeConst,    // for detecting const logic errors
   FunctionCouldBeConst,     // function could be declared as const
   FunctionCouldBeStatic,    // function could be declared as static
   FunctionCouldBeFree,      // function could be declared in namespace
   StaticFunctionViaMember,  // static function invoked via "." or "->" operator
   NonBooleanConditional,    // conditional expression is not a boolean
   EnumTypesDiffer,          // binary operator uses two different enum types
   UseOfTab,                 // tab character in line
   Indentation,              // indentation not a multiple of IndentSize()
   TrailingSpace,            // space after last non-blank character
   AdjacentSpaces,           // extra embedded space in source code
   AddBlankLine,             // add blank line
   RemoveLine,               // remove unnecessary line
   LineLength,               // line length exceeds LineLengthMax() characters
   FunctionNotSorted,        // function definition not in alphabetical order
   HeadingNotStandard,       // lines at top of file do not follow template
   IncludeGuardMisnamed,     // #include guard name is not based on filename
   DebugFtNotInvoked,        // function does not invoke Debug::ft
   DebugFtNotFirst,          // function invokes Debug::ft after first statement
   DebugFtNameMismatch,      // function name for Debug::ft is incorrect
   DebugFtNameDuplicated,    // function name for Debug::ft used previously
   DisplayNotOverridden,     // class does not override Base.Display
   PatchNotOverridden,       // class does not override Object.Patch
   FunctionCouldBeDefaulted, // empty special member function defined
   InitCouldUseConstructor,  // initialization uses oper= instead of constructor
   CouldBeNoexcept,          // function could be tagged noexcept
   ShouldNotBeNoexcept,      // function should not be tagged noexcept
   UseOfSlashAsterisk,       // use of /* */ comment
   RemoveLineBreak,          // next line can be merged within length limit
   CopyCtorConstructsBase,   // copy/move constructor relies on base constructor
   ValueArgumentModified,    // argument passed by value is modified
   ReturnsNonConstMember,    // returns non-const reference or pointer to member
   FunctionCouldBeMember,    // static|free but has an indirect class argument
   ExplicitConstructor,      // constructor need not be tagged explicit
   BitwiseOperatorOnBoolean, // operator | or & used on boolean
   DebugFtCanBeLiteral,      // could pass an inline string literal to Debug::ft
   UnnecessaryCast,          // non-const cast to a base class or the same class
   ExcessiveCast,            // could use static_cast or dynamic_cast
   DataCouldBeFree,          // data could move from header to implementation
   ConstructorNotPrivate,    // singleton should have private constructor
   DestructorNotPrivate,     // singleton should have private destructor
   RedundantScope,           // scope name not required to resolve symbol
   PreprocessorDirective,    // C-style preprocessor directive
   OperatorSpacing,          // add/remove spaces before/after operator
   PunctuationSpacing,       // add/remove spaces before/after punctuation
   CopyCtorNotDeleted,       // base for singletons should delete copy ctor
   CopyOperNotDeleted,       // base for singletons should delete copy operator
   CtorCouldBeDeleted,       // could be namespace but has non-public members
   NoJumpOrFallthrough,      // no jump or [[fallthrough]] above case or default
   OverrideNotSorted,        // override declaration not in alphabetical order
   DataShouldBeStatic,       // non-extern data at .cpp file scope is not static
   Warning_N                 // number of warnings
};

//  Options for the >fix command.
//
struct FixOptions
{
   Warning warning;  // type of warning to fix
   bool prompt;      // whether to prompt before fixing a warning
   bool multiple;    // multiple files being fixed

   FixOptions() : warning(Warning_N), prompt(true), multiple(false) { }
};

//------------------------------------------------------------------------------
//
//  Types of source code lines.
//
enum LineType
{
   CodeLine,              // source code
   BlankLine,             // blank lines
   EmptyComment,          // //
   FileComment,           // comment at top of file, before any code
   RuleComment,           // //# (# = repeated -, =, or /)
   TextComment,           // //  text
   SlashAsteriskComment,  // /*
   OpenBrace,             // {
   CloseBrace,            // }
   CloseBraceSemicolon,   // };
   AccessControl,         // public: protected: private:
   DebugFt,               // Debug::ft(Class_Func);
   FunctionName,          // fn_name Class_Func = "Class.Func";
   IncludeDirective,      // #include
   HashDirective,         // #ifndef #define #endif et al
   UsingStatement,        // using directive or declaration
   AnyLine,               // all lines
   LineType_N             // number of line types
};

//  Inserts a string for TYPE into STREAM.
//
std::ostream& operator<<(std::ostream& stream, LineType type);

//  Attributes of a line type.
//
struct LineTypeAttr
{
   //  The line contains code.
   //
   const bool isCode;

   //  The line contains code whose position is registered when parsing.
   //
   const bool isParsePos;

   //  The line can be merged with another line.
   //
   const bool isMergeable;

   //  The line is considered to be blank.
   //
   const bool isBlank;

   //  A character symbol for the line type.
   //
   const char symbol;

   //  The array that contains the above attributes for each line type.
   //
   static const LineTypeAttr Attrs[LineType_N + 1];
private:
   //  Constructs a line type with the specified attributes.
   //
   LineTypeAttr(bool code, bool pos, bool merge, bool blank, char sym);
};

//  Classifies a line of code (S).  Sets CONT for a line of code that does
//  not end in a semicolon.
//
LineType CalcLineType(std::string& s, bool& cont);

//  Checks a line of code (S), updating WARNINGS with formatting errors.
//
void CheckLine(std::string& s, std::set< Warning >& warnings);

//  Returns true if a S is a bare access control.
//
bool IsAccessControl(const std::string& s);

//  Options for the Display function.
//
enum CodeDisplayOptions
{
   DispFQ,    // display fully qualified name
   DispNS,    // display in namespace view (else in file view)
   DispLF,    // insert optional line feed
   DispNoLF,  // omit line feed
   DispLast,  // set for the last item in a series
   DispCode,  // output will be used to generate code
   DispNoAC,  // omit access control prefix
   DispNoTP,  // omit template parameters definition list
   DispStats  // include statistics (e.g. reads, writes)
};

extern const NodeBase::Flags FQ_Mask;
extern const NodeBase::Flags NS_Mask;
extern const NodeBase::Flags LF_Mask;
extern const NodeBase::Flags NoLF_Mask;
extern const NodeBase::Flags Last_Mask;
extern const NodeBase::Flags Code_Mask;
extern const NodeBase::Flags NoAC_Mask;
extern const NodeBase::Flags NoTP_Mask;
extern const NodeBase::Flags Stats_Mask;

//------------------------------------------------------------------------------
//
//  Editor actions that require a code item to update its location.
//
enum EditorAction
{
   Erased,
   Inserted,
   Pasted
};
}
#endif
