//==============================================================================
//
//  CodeTypes.cpp
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
#include "CodeTypes.h"
#include <ostream>
#include "Cxx.h"
#include "CxxString.h"
#include "Debug.h"
#include "FunctionName.h"

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

fixed_string ALIGNAS_STR          = "alignas";
fixed_string ALIGNOF_STR          = "alignof";
fixed_string ASM_STR              = "asm";
fixed_string AUTO_STR             = "auto";
fixed_string BOOL_STR             = "bool";
fixed_string BREAK_STR            = "break";
fixed_string CATCH_STR            = "catch";
fixed_string CASE_STR             = "case";
fixed_string CHAR_STR             = "char";
fixed_string CHAR16_STR           = "char16_t";
fixed_string CHAR32_STR           = "char32_t";
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
fixed_string GOTO_STR             = "goto";
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
fixed_string STATIC_ASSERT_STR    = "static_assert";
fixed_string STATIC_CAST_STR      = "static_cast";
fixed_string STRUCT_STR           = "struct";
fixed_string SWITCH_STR           = "switch";
fixed_string TEMPLATE_STR         = "template";
fixed_string THIS_STR             = "this";
fixed_string THREAD_LOCAL_STR     = "thread_local";
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
fixed_string VOLATILE_STR         = "volatile";
fixed_string WCHAR_STR            = "wchar_t";
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
fixed_string COMMENT_BEGIN_STR = "/*";
fixed_string COMMENT_END_STR   = "*/";
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
   "source code not in one of the categories below",
   "blank line",
   "blank comment",
   "comment at the top of a file (e.g. for the file's name or license info)",
   "comment followed by repeated characters to draw a rule (e.g. //---- ...)",
   "comment not in one of the categories above (e.g. //  <text>)",
   "C-style comment",
   "bare left brace",
   "bare right brace",
   "bare right brace with semicolon",
   "access control",
   "invocation of Debug::ft",
   "definition of an fn_name",
   "#include directive",
   "preprocessor directive other than #include",
   "using statement",
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

fixed_string FunctionRoleStrings[FuncRole_N + 1] =
{
   "constructor",       // PureCtor
   "destructor",        // PureDtor
   "copy constructor",  // CopyCtor
   "move constructor",  // MoveCtor
   "copy operator",     // CopyOper
   "move operator",     // MoveOper
   "member function",   // FuncOther
   ERROR_STR
};

ostream& operator<<(ostream& stream, FunctionRole role)
{
   if((role >= 0) && (role < FuncRole_N))
      stream << FunctionRoleStrings[role];
   else
      stream << FunctionRoleStrings[FuncRole_N];
   return stream;
}

//------------------------------------------------------------------------------

const bool F = false;
const bool T = true;

//------------------------------------------------------------------------------

LineTypeAttr::LineTypeAttr
   (bool code, bool pos, bool merge, bool blank, char sym) :
   isCode(code),
   isParsePos(pos),
   isMergeable(merge),
   isBlank(blank),
   symbol(sym)
{
}

const LineTypeAttr LineTypeAttr::Attrs[LineType_N + 1] =
{
   //           c  p  m  b
   LineTypeAttr(T, T, T, F, 'c'),  // CodeLine
   LineTypeAttr(F, F, F, T, ' '),  // BlankLine
   LineTypeAttr(F, F, F, T, 'b'),  // EmptyComment
   LineTypeAttr(F, F, F, F, 'f'),  // FileComment
   LineTypeAttr(F, F, F, F, '-'),  // RuleComment
   LineTypeAttr(F, F, F, F, 't'),  // TextComment
   LineTypeAttr(F, F, F, F, '/'),  // SlashAsteriskComment
   LineTypeAttr(T, F, F, F, '{'),  // OpenBrace
   LineTypeAttr(T, F, F, F, '}'),  // CloseBrace
   LineTypeAttr(T, F, F, F, ']'),  // CloseBraceSemicolon
   LineTypeAttr(T, F, F, F, 'a'),  // AccessControl
   LineTypeAttr(T, T, T, F, 'd'),  // DebugFt
   LineTypeAttr(T, T, T, F, 'n'),  // FunctionName
   LineTypeAttr(T, T, F, F, 'i'),  // IncludeDirective
   LineTypeAttr(T, T, F, F, 'h'),  // HashDirective
   LineTypeAttr(T, T, F, F, 'u'),  // UsingStatement
   LineTypeAttr(F, F, F, F, '@'),  // AnyLine
   LineTypeAttr(F, F, F, F, '?')   // LineType_N
};

//------------------------------------------------------------------------------

LineType CalcLineType(string s, bool& cont, std::set< Warning >& warnings)
{
   Debug::ft("CodeTools.CalcLineType");

   cont = false;

   if(s.empty()) return BlankLine;

   //  There is probably a CRLF at the end of the line.
   //
   if(s.back() == CRLF)
   {
      s.pop_back();
      if(s.empty()) return BlankLine;
   }

   //  Flag any tabs and convert them to spaces.
   //
   for(auto pos = s.find(TAB); pos != string::npos; pos = s.find(TAB))
   {
      warnings.insert(UseOfTab);
      s[pos] = SPACE;
   }

   //  Flag and strip trailing spaces.
   //
   auto pos = s.find_first_not_of(SPACE);

   if(pos == string::npos)
   {
      warnings.insert(TrailingSpace);
      return BlankLine;
   }

   if(pos > 0) s.erase(0, pos);

   while(s.back() == SPACE)
   {
      warnings.insert(TrailingSpace);
      s.pop_back();
   }

   auto length = s.size();

   //  Look for lines that contain nothing but a brace (or brace and semicolon).
   //
   if((s[0] == '{') && (length == 1)) return OpenBrace;

   if(s[0] == '}')
   {
      if(length == 1) return CloseBrace;
      if((s[1] == ';') && (length == 2)) return CloseBraceSemicolon;
   }

   //  Classify lines that contain only a // comment.
   //
   size_t slashSlashPos = s.find(COMMENT_STR);

   if(slashSlashPos == 0)
   {
      if(length == 2) return EmptyComment;  //
      if(s[2] == '-') return RuleComment;   //-
      if(s[2] == '=') return RuleComment;   //=
      if(s[2] == '/') return RuleComment;   ///
      return TextComment;                   //  text
   }

   //  Flag a /* comment and see if it ends on the same line.
   //
   pos = FindSubstr(s, COMMENT_BEGIN_STR);

   if(pos != string::npos)
   {
      warnings.insert(UseOfSlashAsterisk);
      if(pos == 0) return SlashAsteriskComment;
   }

   //  Look for preprocessor directives (e.g. #include, #ifndef).
   //
   if(s[0] == '#')
   {
      pos = s.find(HASH_INCLUDE_STR);
      if(pos == 0) return IncludeDirective;
      return HashDirective;
   }

   //  Look for using statements.
   //
   if(s.find("using ") == 0)
   {
      cont = (LastCodeChar(s, slashSlashPos) != ';');
      return UsingStatement;
   }

   //  Look for access controls.
   //
   if(IsAccessControl(s)) return AccessControl;

   //  Look for invocations of Debug::ft and its variants.
   //
   if(FindSubstr(s, "Debug::ft(") != string::npos) return DebugFt;
   if(FindSubstr(s, "Debug::ftnt(") != string::npos) return DebugFt;
   if(FindSubstr(s, "Debug::noft(") != string::npos) return DebugFt;

   //  Look for strings that provide function names for Debug::ft.  These
   //  have the format
   //    fn_name ClassName_FunctionName = "ClassName.FunctionName";
   //  with an endline after the '=' if the line would exceed LineLengthMax
   //  characters.
   //
   string type(FunctionName::TypeStr);
   type.push_back(SPACE);

   while(true)
   {
      if(s.find(type) != 0) break;
      auto begin1 = s.find_first_not_of(SPACE, type.size());
      if(begin1 == string::npos) break;
      auto under = s.find('_', begin1);
      if(under == string::npos) break;
      auto equals = s.find('=', under);
      if(equals == string::npos) break;

      if(LastCodeChar(s, slashSlashPos) == '=')
      {
         cont = true;
         return FunctionName;
      }

      auto end1 = s.find_first_not_of(ValidNextChars, under);
      if(end1 == string::npos) break;
      auto begin2 = s.find(QUOTE, equals);
      if(begin2 == string::npos) break;
      auto dot = s.find('.', begin2);
      if(dot == string::npos) break;
      auto end2 = s.find(QUOTE, dot);
      if(end2 == string::npos) break;

      auto front = under - begin1;
      if(s.substr(begin1, front) == s.substr(begin2 + 1, front))
      {
         return FunctionName;
      }
      break;
   }

   pos = FindSubstr(s, "  ");

   if(pos != string::npos)
   {
      auto next = s.find_first_not_of(SPACE, pos);

      if((next != string::npos) && (next != slashSlashPos) && (s[next] != '='))
      {
         warnings.insert(AdjacentSpaces);
      }
   }

   cont = (LastCodeChar(s, slashSlashPos) != ';');
   return CodeLine;
}

//------------------------------------------------------------------------------

size_t IndentSize()
{
   return 3;
}

//------------------------------------------------------------------------------

bool IsAccessControl(const std::string& s)
{
   //  If S is an access control, check that nothing follows it.
   //
   auto acc = FindAccessControl(s);
   if(acc == Cxx::Access_N) return false;
   auto pos = s.find(':');
   if(pos == string::npos) return false;
   return (s.find_first_not_of(WhitespaceChars, pos + 1) == string::npos);
}

//------------------------------------------------------------------------------

size_t LineLengthMax()
{
   return 80;
}

//==============================================================================

SymbolView::SymbolView() :
   accessibility(Inaccessible),
   match(Compatible),
   defts(false),
   using_(false),
   friend_(false),
   resolved(false),
   distance(0)
{
}

//------------------------------------------------------------------------------

SymbolView::SymbolView(Accessibility a,
   TypeMatch m, bool c, bool u, bool f, bool r, Distance d) :
   accessibility(a),
   match(m),
   defts(c),
   using_(u),
   friend_(f),
   resolved(r),
   distance(d)
{
}

//------------------------------------------------------------------------------

const SymbolView NotAccessible
   (Inaccessible, Compatible, false, false, false, true, 0);
const SymbolView DeclaredGlobally
   (Unrestricted, Compatible, false, false, false, true, 0);
const SymbolView DeclaredLocally
   (Declared, Compatible, false, false, false, true, 0);
}

