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
const string WhitespaceChars(" \n\t\v\f\r");
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
fixed_string FINAL_STR            = "final";
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
fixed_string COMMENT_BEGIN_STR = "/*";
fixed_string COMMENT_STR       = "//";
fixed_string ELLIPSES_STR      = "...";
fixed_string LOCALS_STR        = "$locals";  // name for code blocks
fixed_string NULL_STR          = "NULL";

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

//------------------------------------------------------------------------------

fixed_string LineTypeStrings[LineType_N + 1] =
{
   "source code",
   "blank",
   COMMENT_STR,
   "file //",
   "separator //",
   "tagged //",
   "text //",
   COMMENT_BEGIN_STR,
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

ostream& operator<<(ostream& stream, LineType type)
{
   if((type >= 0) && (type < LineType_N))
      stream << LineTypeStrings[type];
   else
      stream << LineTypeStrings[LineType_N];
   return stream;
}

//------------------------------------------------------------------------------

const bool F = false;
const bool T = true;

//------------------------------------------------------------------------------

LineTypeAttr::LineTypeAttr(bool code, bool exe, bool merge, bool blank) :
   isCode(code),
   isExecutable(exe),
   isMergeable(merge),
   isBlank(blank)
{
}

const LineTypeAttr LineTypeAttr::Attrs[LineType_N + 1] =
{
   //           c  x  m  b
   LineTypeAttr(T, T, T, F),  // Code
   LineTypeAttr(F, F, F, T),  // Blank
   LineTypeAttr(F, F, F, T),  // EmptyComment
   LineTypeAttr(F, F, F, F),  // FileComment
   LineTypeAttr(F, F, F, F),  // SeparatorComment
   LineTypeAttr(F, F, F, F),  // TaggedComment
   LineTypeAttr(F, F, F, F),  // TextComment
   LineTypeAttr(F, F, F, F),  // SlashAsteriskComment
   LineTypeAttr(T, F, F, F),  // OpenBrace
   LineTypeAttr(T, F, F, F),  // CloseBrace
   LineTypeAttr(T, F, F, F),  // CloseBraceSemicolon
   LineTypeAttr(T, T, T, F),  // DebugFt
   LineTypeAttr(T, T, T, F),  // FunctionName
   LineTypeAttr(T, T, T, F),  // FunctionNameSplit
   LineTypeAttr(T, T, F, F),  // IncludeDirective
   LineTypeAttr(T, T, F, F),  // HashDirective
   LineTypeAttr(T, T, F, F),  // UsingDirective
   LineTypeAttr(F, F, F, F),  // AnyLne
   LineTypeAttr(F, F, F, F)   // LineType_N
};

//------------------------------------------------------------------------------

bool LinesCanBeMerged
   (const string& line1, size_t begin1, size_t end1,
    const string& line2, size_t begin2, size_t end2)
{
   //  LINE1 must not end in a trailing comment, semicolon, colon, or right
   //  brace and must not start with an "if" or "else".  LINE2 must end with
   //  a semicolon.  If LINE2 doesn't start with a left parenthesis, a space
   //  will also have to be inserted when merging.
   //
   if(line1.at(end1) == ';') return false;
   if(line1.at(end1) == ':') return false;
   if(line1.at(end1) == '}') return false;
   if(line2.at(end2) != ';') return false;
   if(line1.find(COMMENT_STR, begin1) < end1) return false;
   auto first1 = line1.find_first_not_of(WhitespaceChars, begin1);
   if(line1.find(IF_STR, first1) == first1) return false;
   if(line1.find(ELSE_STR, first1) == first1) return false;
   begin2 = line2.find_first_not_of(WhitespaceChars, begin2);
   auto size = (end1 - begin1 + 1) + (end2 - begin2 + 1);
   if(line2.at(begin2) != '(') ++size;
   return (size <= LINE_LENGTH_MAX);
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

