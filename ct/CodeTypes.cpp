//==============================================================================
//
//  CodeTypes.cpp
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
//
#include "CodeTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CodeTools
{
const string ValidFirstChars
   ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_#~");
const string ValidNextChars
   ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_#0123456789");
const string ValidTemplateSpecChars(ValidNextChars + "<>,*[]: ");
const string ValidOpChars(".=:(),!<>&|+-[]~*/%^?");
const string ValidIntChars("0123456789.XxUuLlEe");
const string ValidIntDigits("0123456789");
const string ValidHexDigits("0123456789abcdefABCDEF");
const string ValidOctDigits("01234567");
const string WhitespaceChars(" \t\n\v\f\r");
const string SingleRule(COMMENT_STR + string(78, '-'));
const string DoubleRule(COMMENT_STR + string(78, '='));

//------------------------------------------------------------------------------

fixed_string AUTO_STR             = "auto";
fixed_string BOOL_STR             = "bool";
fixed_string BREAK_STR            = "break";
fixed_string CATCH_STR            = "catch";
fixed_string CASE_STR             = "case";
fixed_string CHAR_STR             = "char";
fixed_string CLASS_STR            = "class";
fixed_string CONST_STR            = "const";
fixed_string CONST_CAST_STR       = "const_cast";
fixed_string CONSTEXPR_STR        = "constexpr";
fixed_string CONTINUE_STR         = "continue";
fixed_string DEFAULT_STR          = "default";
fixed_string DELETE_STR           = "delete";
fixed_string DELETE_ARRAY_STR     = "delete[]";
fixed_string DOUBLE_STR           = "double";
fixed_string DYNAMIC_CAST_STR     = "dynamic_cast";
fixed_string DO_STR               = "do";
fixed_string ELSE_STR             = "else";
fixed_string ENUM_STR             = "enum";
fixed_string EXPLICIT_STR         = "explicit";
fixed_string EXTERN_STR           = "extern";
fixed_string FALSE_STR            = "false";
fixed_string FLOAT_STR            = "float";
fixed_string FOR_STR              = "for";
fixed_string FRIEND_STR           = "friend";
fixed_string IF_STR               = "if";
fixed_string INLINE_STR           = "inline";
fixed_string INT_STR              = "int";
fixed_string LONG_STR             = "long";
fixed_string MUTABLE_STR          = "mutable";
fixed_string NAMESPACE_STR        = "namespace";
fixed_string NEW_STR              = "new";
fixed_string NEW_ARRAY_STR        = "new[]";
fixed_string NOEXCEPT_STR         = "noexcept";
fixed_string NULLPTR_STR          = "nullptr";
fixed_string NULLPTR_T_STR        = "nullptr_t";
fixed_string OPERATOR_STR         = "operator";
fixed_string OVERRIDE_STR         = "override";
fixed_string PRIVATE_STR          = "private";
fixed_string PROTECTED_STR        = "protected";
fixed_string PUBLIC_STR           = "public";
fixed_string REINTERPRET_CAST_STR = "reinterpret_cast";
fixed_string RETURN_STR           = "return";
fixed_string SHORT_STR            = "short";
fixed_string SIGNED_STR           = "signed";
fixed_string SIZEOF_STR           = "sizeof";
fixed_string STATIC_STR           = "static";
fixed_string STATIC_CAST_STR      = "static_cast";
fixed_string STRUCT_STR           = "struct";
fixed_string SWITCH_STR           = "switch";
fixed_string TEMPLATE_STR         = "template";
fixed_string THIS_STR             = "this";
fixed_string THROW_STR            = "throw";
fixed_string TRUE_STR             = "true";
fixed_string TRY_STR              = "try";
fixed_string TYPEDEF_STR          = "typedef";
fixed_string TYPEID_STR           = "typeid";
fixed_string TYPENAME_STR         = "typename";
fixed_string UNION_STR            = "union";
fixed_string UNSIGNED_STR         = "unsigned";
fixed_string USING_STR            = "using";
fixed_string VIRTUAL_STR          = "virtual";
fixed_string VOID_STR             = "void";
fixed_string WHILE_STR            = "while";

fixed_string DEFINED_STR      = "defined";
fixed_string HASH_DEFINE_STR  = "#define";
fixed_string HASH_ELIF_STR    = "#elif";
fixed_string HASH_ELSE_STR    = "#else";
fixed_string HASH_ENDIF_STR   = "#endif";
fixed_string HASH_ERROR_STR   = "#error";
fixed_string HASH_IF_STR      = "#if";
fixed_string HASH_IFDEF_STR   = "#ifdef";
fixed_string HASH_IFNDEF_STR  = "#ifndef";
fixed_string HASH_INCLUDE_STR = "#include";
fixed_string HASH_LINE_STR    = "#line";
fixed_string HASH_PRAGMA_STR  = "#pragma";
fixed_string HASH_UNDEF_STR   = "#undef";

fixed_string ARRAY_STR         = "[]";
fixed_string COMMENT_END_STR   = "*/";
fixed_string COMMENT_START_STR = "/*";
fixed_string COMMENT_STR       = "//";
fixed_string ELLIPSES_STR      = "...";
fixed_string LOCALS_STR        = "$locals";  // name for code blocks

//------------------------------------------------------------------------------

const Flags FQ_Mask = Flags(1 << DispFQ);
const Flags NS_Mask = Flags(1 << DispNS);
const Flags LF_Mask = Flags(1 << DispLF);
const Flags NoLF_Mask = Flags(1 << DispNoLF);
const Flags Last_Mask = Flags(1 << DispLast);
const Flags Code_Mask = Flags(1 << DispCode);
const Flags NoAC_Mask = Flags(1 << DispNoAC);
const Flags NoTP_Mask = Flags(1 << DispNoTP);
const Flags Stats_Mask = Flags(1 << DispStats);

const uint8_t Indent_Size = 3;

//------------------------------------------------------------------------------

fixed_string LineTypeStrings[LineType_N + 1] =
{
   "source code",
   "blank",
   COMMENT_STR,
   "license //",
   "separator //",
   "tagged //",
   "text //",
   COMMENT_START_STR,
   "{",
   "}",
   "};",
   "Debug::ft",
   "fn_name",
   "...split",
   HASH_INCLUDE_STR,
   "#<other>",
   USING_STR,
   "TOTAL",
   ERROR_STR
};

ostream& operator<<(std::ostream& stream, LineType type)
{
   if((type >= 0) && (type < LineType_N))
      stream << LineTypeStrings[type];
   else
      stream << LineTypeStrings[LineType_N];
   return stream;
}

//------------------------------------------------------------------------------

fixed_string WarningStrings[Warning_N + 1] =
{
   "C-style comment",
   "NULL",
   "Pointer tag ('*') detached from type",
   "Reference tag ('&') detached from type",
   "C-style cast",
   "Functional cast",
   "reinterpret_cast",
   "Cast down the inheritance hierarchy",
   "Cast removes const qualification",
   "Pointer arithmetic",
   "Semicolon not required",
   "Redundant const in type specification",
   "#define appears within a class or function",
   "#include appears within a class or function",
   "No #include guard found",
   "#include not sorted in standard order",
   "#include duplicated",
   "Add #include directive",
   "Remove #include directive",
   "Header relies on using statement via #include",
   "Using statement in header",
   "Using statement duplicated",
   "Add using statement",
   "Remove using statement",
   "Add forward declaration",
   "Remove forward declaration",
   "Unused argument",
   "Unused class",
   "Unused data",
   "Unused enum",
   "Unused enumerator",
   "Unused forward declaration",
   "Unused friend declaration",
   "Unused function",
   "Unused typedef",
   "Unused using statement",
   "No referent for forward declaration",
   "No referent for friend declaration",
   "Indirect reference relies on friend, not forward, declaration",
   "Member hides inherited name",
   "Class could be namespace",
   "Class could be struct",
   "Struct could be class",
   "Redundant access control",
   "Member could be private",
   "Member could be protected",
   "Typedef of pointer type",
   "Unnamed enum",
   "Global data initialization not found",
   "Data is init-only",
   "Data is write-only",
   "Global static data",
   "Data is not private",
   "DATA CANNOT BE CONST",
   "DATA CANNOT BE CONST POINTER",
   "Data could be const",
   "Data could be const pointer",
   "Data need not be mutable",
   "Default constructor invoked: POD members not initialized",
   "Default constructor invoked",
   "Default copy constructor invoked",
   "Default assignment operator invoked",
   "Base class constructor is public",
   "Single-argument constructor is not explicit",
   "Member not included in member initialization list",
   "Member not sorted in standard order in member initialization list",
   "Default destructor invoked",
   "Base class virtual destructor is not public",
   "Base class non-virtual destructor is public",
   "Virtual function in own class invoked by constructor or destructor",
   "Destructor defined, but not copy constructor",
   "Destructor defined, but not copy operator",
   "Copy constructor defined, but not copy operator",
   "Copy operator defined, but not copy constructor",
   "Overloading operator && or ||",
   "Function not implemented",
   "Pure virtual function not implemented",
   "Virtual function is public",
   "Virtual function is overloaded",
   "Virtual function has no overrides",
   "Function should be tagged as virtual",
   "Function should be tagged as override",
   "\"(void)\" as function argument",
   "Unnamed argument",
   "Adjacent arguments have the same type",
   "Definition renames argument in declaration",
   "Override renames argument in direct base class",
   "Virtual function defines default argument",
   "ARGUMENT CANNOT BE CONST",
   "Object could be passed by const reference",
   "Argument could be const",
   "FUNCTION CANNOT BE CONST",
   "Function could be const",
   "Function could be static",
   "Function could be free",
   "Static function invoked via operator \".\" or \"->\"",
   "Non-boolean in conditional expression",
   "Arguments to binary operator have different enum types",
   "Tab character",
   "Line indentation is not a multiple of the standard value",
   "Line contains trailing space",
   "Line contains adjacent spaces",
   "Insertion of blank line recommended",
   "Deletion of blank line recommended",
   "Line length exceeds the standard maximum",
   "Function not sorted in standard order",
   "File heading is not standard",
   "Name of #include guard is not standard",
   "Function does not invoke Debug::ft",
   "Function does not invoke Debug::ft as first statement",
   "Function name string for Debug::ft is not standard",
   "Override of Base.Display not found",
   "Override of Object.Patch not found",
   ERROR_STR
};

//------------------------------------------------------------------------------

bool IsUnusedItemWarning(Warning warning)
{
   switch(warning)
   {
   case ArgumentUnused:
   case ClassUnused:
   case DataUnused:
   case EnumUnused:
   case EnumeratorUnused:
   case ForwardUnused:
   case FriendUnused:
   case FunctionUnused:
   case TypedefUnused:
   case UsingUnused:
      return true;
   }

   return false;
}

//------------------------------------------------------------------------------

ostream& operator<<(std::ostream& stream, Warning warning)
{
   if((warning >= 0) && (warning < Warning_N))
      stream << WarningStrings[warning];
   else
      stream << WarningStrings[Warning_N];
   return stream;
}

//==============================================================================

SymbolView::SymbolView() :
   accessibility(Inaccessible),
   match(Compatible),
   using_(false),
   friend_(false),
   resolved(false),
   distance(0)
{
}

//------------------------------------------------------------------------------

SymbolView::SymbolView(Accessibility a,
   TypeMatch m, bool u, bool f, bool r, Distance d) :
   accessibility(a),
   match(m),
   using_(u),
   friend_(f),
   resolved(r),
   distance(d)
{
}

//------------------------------------------------------------------------------

const SymbolView NotAccessible
   (Inaccessible, Compatible, false, false, true, 0);
const SymbolView DeclaredGlobally
   (Unrestricted, Compatible, false, false, true, 0);
const SymbolView DeclaredLocally
   (Declared,Compatible, false, false, true, 0);
}

