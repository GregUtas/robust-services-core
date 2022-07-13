//==============================================================================
//
//  Cxx.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "Cxx.h"
#include <cctype>
#include <cstring>
#include <iomanip>
#include <ios>
#include <sstream>
#include "CodeFile.h"
#include "CxxNamed.h"
#include "CxxScope.h"
#include "Debug.h"

using std::ostream;
using std::setw;
using std::string;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
string CharString(uint32_t c, bool s)
{
   switch(c)
   {
   case 0x00: return "\\0";
   case 0x07: return "\\a";
   case 0x08: return "\\b";
   case 0x0c: return "\\f";
   case 0x0a: return "\\n";
   case 0x0d: return "\\r";
   case 0x09: return "\\t";
   case 0x0b: return "\\v";
   case BACKSLASH: return "\\\\";
   case QUOTE:
      if(s) return "\\\"";
      break;
   case APOSTROPHE:
      if(!s) return "\\'";
      break;
   }

   if((c >= 32) && (c <= 126))
   {
      return string(1, c);  // displayable, not escaped
   }

   std::ostringstream stream;
   stream << std::hex << std::setfill('0');

   if(c <= UINT8_MAX)
      stream << BACKSLASH << 'x' << setw(2) << uint32_t(c);
   else if(c <= UINT16_MAX)
      stream << BACKSLASH << 'u' << setw(4) << uint32_t(c);
   else
      stream << BACKSLASH << 'U' << setw(8) << uint32_t(c);

   return stream.str();
}

//------------------------------------------------------------------------------
//
//  Erases, from SET, items defined within DEFN.
//
static void EraseLocals(const CxxScope* defn, CxxNamedSet& set)
{
   Debug::ft("CodeTools.EraseLocals");

   size_t begin, end;
   defn->GetSpan2(begin, end);
   auto file = defn->GetFile();

   for(auto i = set.cbegin(); i != set.cend(); NO_OP)
   {
      auto pos = (*i)->GetPos();

      if(((*i)->GetFile() == file) && (pos >= begin) && (pos <= end))
         i = set.erase(i);
      else
         ++i;
   }
}

//------------------------------------------------------------------------------

fixed_string AccessStrings[Cxx::Access_N + 1] =
{
   PRIVATE_STR,
   PROTECTED_STR,
   PUBLIC_STR,
   ERROR_STR
};

Cxx::Access FindAccessControl(const std::string& s)
{
   auto pos = s.find_first_not_of(WhitespaceChars);
   if(pos == string::npos) return Cxx::Access_N;

   for(int acc = Cxx::Private; acc != Cxx::Access_N; ++acc)
   {
      auto kwd = AccessStrings[acc];

      if(s.compare(pos, strlen(kwd), kwd) == 0)
      {
         pos = s.find_first_not_of(WhitespaceChars, pos + strlen(kwd));
         if(pos == string::npos) return Cxx::Access_N;
         if(s[pos] != ':') return Cxx::Access_N;
         return static_cast<Cxx::Access>(acc);
      }
   }

   return Cxx::Access_N;
}

//------------------------------------------------------------------------------

CxxUsageSets GetItemUsages(CxxScope* decl, bool extOnly)
{
   Debug::ft("CodeTools.GetItemUsages");

   CxxUsageSets usages;
   decl->GetUsages(*decl->GetFile(), usages);

   auto defn = decl->GetMate();
   if(defn != nullptr)
   {
      auto file = defn->GetFile();
      defn->GetUsages(*file, usages);

      if(extOnly)
      {
         file->EraseInternals(usages.directs);
         file->EraseInternals(usages.indirects);
      }
      else
      {
         EraseLocals(defn, usages.directs);
         EraseLocals(defn, usages.indirects);
      }
   }

   usages.directs.erase(decl);
   usages.indirects.erase(decl);
   return usages;
}

//------------------------------------------------------------------------------

ostream& operator<<(ostream& stream, Cxx::Access access)
{
   if(access < Cxx::Access_N)
      stream << AccessStrings[access];
   else
      stream << AccessStrings[Cxx::Access_N];
   return stream;
}

//------------------------------------------------------------------------------

fixed_string ClassTagStrings[Cxx::ClassTag_N + 1] =
{
   TYPENAME_STR,
   CLASS_STR,
   STRUCT_STR,
   UNION_STR,
   ERROR_STR
};

ostream& operator<<(ostream& stream, Cxx::ClassTag tag)
{
   if(tag < Cxx::ClassTag_N)
      stream << ClassTagStrings[tag];
   else
      stream << ClassTagStrings[Cxx::ClassTag_N];
   return stream;
}

//------------------------------------------------------------------------------

fixed_string EncodingStrings[Cxx::Encoding_N + 1] =
{
   "",
   "u8",
   "u",
   "U",
   "L",
   ERROR_STR
};

ostream& operator<<(ostream& stream, Cxx::Encoding code)
{
   if((code >= 0) && (code < Cxx::Encoding_N))
      stream << EncodingStrings[code];
   else
      stream << EncodingStrings[Cxx::Encoding_N];
   return stream;
}

//------------------------------------------------------------------------------

fixed_string SpecifierStrings[Cxx::Specifier_N + 1] =
{
   EMPTY_STR,
   CLASS_STR,
   STRUCT_STR,
   UNION_STR,
   ENUM_STR,
   ERROR_STR
};

ostream& operator<<(ostream& stream, Cxx::Specifier specifier)
{
   if(specifier < Cxx::Specifier_N)
      stream << SpecifierStrings[specifier];
   else
      stream << SpecifierStrings[Cxx::Specifier_N];
   return stream;
}

//------------------------------------------------------------------------------

Cxx::DirectiveTablePtr Cxx::Directives = nullptr;
Cxx::KeywordTablePtr Cxx::Keywords = nullptr;
Cxx::OperatorTablePtr Cxx::CxxOps = nullptr;
Cxx::OperatorTablePtr Cxx::PreOps = nullptr;
Cxx::OperatorTablePtr Cxx::Reserved = nullptr;
Cxx::TypesTablePtr Cxx::Types = nullptr;

//------------------------------------------------------------------------------

Cxx::Operator Cxx::GetReserved(const string& name)
{
   Debug::ft("Cxx.GetReserved");

   //  See if NAME matches one of a selected group of reserved words.
   //
   auto match = Reserved->find(name);
   if(match != Reserved->cend()) return match->second;
   return NIL_OPERATOR;
}

//------------------------------------------------------------------------------

Cxx::Type Cxx::GetType(const string& name)
{
   Debug::ft("Cxx.GetType");

   auto match = Types->find(name);
   if(match != Types->cend()) return match->second;
   return NIL_TYPE;
}

//------------------------------------------------------------------------------

void Cxx::Initialize()
{
   Debug::ft("Cxx.Initialize");

   Directives.reset(new DirectiveTable);
   Directives->insert(DirectivePair(HASH_DEFINE_STR, HASH_DEFINE));
   Directives->insert(DirectivePair(HASH_ELIF_STR, HASH_ELIF));
   Directives->insert(DirectivePair(HASH_ELSE_STR, HASH_ELSE));
   Directives->insert(DirectivePair(HASH_ENDIF_STR, HASH_ENDIF));
   Directives->insert(DirectivePair(HASH_ERROR_STR, HASH_ERROR));
   Directives->insert(DirectivePair(HASH_IF_STR, HASH_IF));
   Directives->insert(DirectivePair(HASH_IFDEF_STR, HASH_IFDEF));
   Directives->insert(DirectivePair(HASH_IFNDEF_STR, HASH_IFNDEF));
   Directives->insert(DirectivePair(HASH_INCLUDE_STR, HASH_INCLUDE));
   Directives->insert(DirectivePair(HASH_LINE_STR, HASH_LINE));
   Directives->insert(DirectivePair(HASH_PRAGMA_STR, HASH_PRAGMA));
   Directives->insert(DirectivePair(HASH_UNDEF_STR, HASH_UNDEF));

   Keywords.reset(new KeywordTable);
   Keywords->insert(KeywordPair(ALIGNAS_STR, ALIGNAS));
   Keywords->insert(KeywordPair(ASM_STR, ASM));
   Keywords->insert(KeywordPair(AUTO_STR, AUTO));
   Keywords->insert(KeywordPair(BREAK_STR, BREAK));
   Keywords->insert(KeywordPair(CASE_STR, CASE));
   Keywords->insert(KeywordPair(CLASS_STR, CLASS));
   Keywords->insert(KeywordPair(CONST_STR, CONST));
   Keywords->insert(KeywordPair(CONSTEXPR_STR, CONSTEXPR));
   Keywords->insert(KeywordPair(CONTINUE_STR, CONTINUE));
   Keywords->insert(KeywordPair(DEFAULT_STR, DEFAULT));
   Keywords->insert(KeywordPair(DO_STR, DO));
   Keywords->insert(KeywordPair(ENUM_STR, ENUM));
   Keywords->insert(KeywordPair(EXPLICIT_STR, EXPLICIT));
   Keywords->insert(KeywordPair(EXTERN_STR, EXTERN));
   Keywords->insert(KeywordPair(FINAL_STR, FINAL));
   Keywords->insert(KeywordPair(FOR_STR, FOR));
   Keywords->insert(KeywordPair(FRIEND_STR, FRIEND));
   Keywords->insert(KeywordPair(GOTO_STR, GOTO));
   Keywords->insert(KeywordPair(IF_STR, IF));
   Keywords->insert(KeywordPair(INLINE_STR, INLINE));
   Keywords->insert(KeywordPair(MUTABLE_STR, MUTABLE));
   Keywords->insert(KeywordPair(NAMESPACE_STR, NAMESPACE));
   Keywords->insert(KeywordPair(OPERATOR_STR, OPERATOR));
   Keywords->insert(KeywordPair(OVERRIDE_STR, OVERRIDE));
   Keywords->insert(KeywordPair(PRIVATE_STR, PRIVATE));
   Keywords->insert(KeywordPair(PROTECTED_STR, PROTECTED));
   Keywords->insert(KeywordPair(PUBLIC_STR, PUBLIC));
   Keywords->insert(KeywordPair(RETURN_STR, RETURN));
   Keywords->insert(KeywordPair(STATIC_ASSERT_STR, STATIC_ASSERT));
   Keywords->insert(KeywordPair(STATIC_STR, STATIC));
   Keywords->insert(KeywordPair(STRUCT_STR, STRUCT));
   Keywords->insert(KeywordPair(SWITCH_STR, SWITCH));
   Keywords->insert(KeywordPair(TEMPLATE_STR, TEMPLATE));
   Keywords->insert(KeywordPair(THREAD_LOCAL_STR, THREAD_LOCAL));
   Keywords->insert(KeywordPair(TRY_STR, TRY));
   Keywords->insert(KeywordPair(TYPEDEF_STR, TYPEDEF));
   Keywords->insert(KeywordPair(UNION_STR, UNION));
   Keywords->insert(KeywordPair(USING_STR, USING));
   Keywords->insert(KeywordPair(VIRTUAL_STR, VIRTUAL));
   Keywords->insert(KeywordPair(VOLATILE_STR, VOLATILE));
   Keywords->insert(KeywordPair(WHILE_STR, WHILE));

   //  Each string can only have one entry in a hash table.  If a string
   //  is ambiguous, it maps to the operator with the highest precedence,
   //  and other interpretations are commented out.  The parser resolves
   //  the ambiguity.
   //
   CxxOps.reset(new OperatorTable);
   CxxOps->insert(OperatorPair(SCOPE_STR, SCOPE_RESOLUTION));
   CxxOps->insert(OperatorPair(".", REFERENCE_SELECT));
   CxxOps->insert(OperatorPair("->", POINTER_SELECT));
   CxxOps->insert(OperatorPair("[", ARRAY_SUBSCRIPT));
   CxxOps->insert(OperatorPair("(", FUNCTION_CALL));
   CxxOps->insert(OperatorPair("++", POSTFIX_INCREMENT));
   CxxOps->insert(OperatorPair("--", POSTFIX_DECREMENT));
   CxxOps->insert(OperatorPair(TYPEID_STR, TYPE_NAME));
   CxxOps->insert(OperatorPair(CONST_CAST_STR, CONST_CAST));
   CxxOps->insert(OperatorPair(DYNAMIC_CAST_STR, DYNAMIC_CAST));
   CxxOps->insert(OperatorPair(REINTERPRET_CAST_STR, REINTERPRET_CAST));
   CxxOps->insert(OperatorPair(STATIC_CAST_STR, STATIC_CAST));
   CxxOps->insert(OperatorPair(SIZEOF_STR, SIZEOF_TYPE));
   CxxOps->insert(OperatorPair(ALIGNOF_STR, ALIGNOF_TYPE));
   CxxOps->insert(OperatorPair(NOEXCEPT_STR, NOEXCEPT));
// CxxOps->insert(OperatorPair("++", Cxx::PREFIX_INCREMENT));
// CxxOps->insert(OperatorPair("--", Cxx::PREFIX_DECREMENT));
   CxxOps->insert(OperatorPair("~", ONES_COMPLEMENT));
   CxxOps->insert(OperatorPair("!", LOGICAL_NOT));
   CxxOps->insert(OperatorPair("+", UNARY_PLUS));
   CxxOps->insert(OperatorPair("-", UNARY_MINUS));
   CxxOps->insert(OperatorPair("&", ADDRESS_OF));
   CxxOps->insert(OperatorPair("*", INDIRECTION));
   CxxOps->insert(OperatorPair(NEW_STR, OBJECT_CREATE));
   CxxOps->insert(OperatorPair(NEW_ARRAY_STR, OBJECT_CREATE_ARRAY));
   CxxOps->insert(OperatorPair(DELETE_STR, OBJECT_DELETE));
   CxxOps->insert(OperatorPair(DELETE_ARRAY_STR, OBJECT_DELETE_ARRAY));
// CxxOps->insert(OperatorPair("(", Cxx::CAST));
   CxxOps->insert(OperatorPair(".*", REFERENCE_SELECT_MEMBER));
   CxxOps->insert(OperatorPair("->*", POINTER_SELECT_MEMBER));
// CxxOps->insert(OperatorPair("*", Cxx::MULTIPLY));
   CxxOps->insert(OperatorPair("/", DIVIDE));
   CxxOps->insert(OperatorPair("%", MODULO));
// CxxOps->insert(OperatorPair("+", Cxx::ADD));
// CxxOps->insert(OperatorPair("-", Cxx::SUBTRACT));
   CxxOps->insert(OperatorPair("<<", LEFT_SHIFT));
   CxxOps->insert(OperatorPair(">>", RIGHT_SHIFT));
   CxxOps->insert(OperatorPair("<", LESS));
   CxxOps->insert(OperatorPair("<=", LESS_OR_EQUAL));
   CxxOps->insert(OperatorPair(">", GREATER));
   CxxOps->insert(OperatorPair(">=", GREATER_OR_EQUAL));
   CxxOps->insert(OperatorPair("==", EQUALITY));
   CxxOps->insert(OperatorPair("!=", INEQUALITY));
// CxxOps->insert(OperatorPair("&", Cxx::BITWISE_AND));
   CxxOps->insert(OperatorPair("^", BITWISE_XOR));
   CxxOps->insert(OperatorPair("|", BITWISE_OR));
   CxxOps->insert(OperatorPair("&&", LOGICAL_AND));
   CxxOps->insert(OperatorPair("||", LOGICAL_OR));
   CxxOps->insert(OperatorPair("?", CONDITIONAL));
   CxxOps->insert(OperatorPair("=", ASSIGN));
   CxxOps->insert(OperatorPair("*=", MULTIPLY_ASSIGN));
   CxxOps->insert(OperatorPair("/=", DIVIDE_ASSIGN));
   CxxOps->insert(OperatorPair("%=", MODULO_ASSIGN));
   CxxOps->insert(OperatorPair("+=", ADD_ASSIGN));
   CxxOps->insert(OperatorPair("-=", SUBTRACT_ASSIGN));
   CxxOps->insert(OperatorPair("<<=", LEFT_SHIFT_ASSIGN));
   CxxOps->insert(OperatorPair(">>=", RIGHT_SHIFT_ASSIGN));
   CxxOps->insert(OperatorPair("&=", BITWISE_AND_ASSIGN));
   CxxOps->insert(OperatorPair("^=", BITWISE_XOR_ASSIGN));
   CxxOps->insert(OperatorPair("|=", BITWISE_OR_ASSIGN));
   CxxOps->insert(OperatorPair(THROW_STR, THROW));
   CxxOps->insert(OperatorPair(",", STATEMENT_SEPARATOR));

   PreOps.reset(new OperatorTable);
   PreOps->insert(OperatorPair("[", ARRAY_SUBSCRIPT));
   PreOps->insert(OperatorPair("(", FUNCTION_CALL));
   PreOps->insert(OperatorPair(DEFINED_STR, DEFINED));
   PreOps->insert(OperatorPair("~", ONES_COMPLEMENT));
   PreOps->insert(OperatorPair("!", LOGICAL_NOT));
   PreOps->insert(OperatorPair("+", UNARY_PLUS));
   PreOps->insert(OperatorPair("-", UNARY_MINUS));
   PreOps->insert(OperatorPair("*", MULTIPLY));
   PreOps->insert(OperatorPair("/", DIVIDE));
   PreOps->insert(OperatorPair("%", MODULO));
   PreOps->insert(OperatorPair("+", ADD));
   PreOps->insert(OperatorPair("-", SUBTRACT));
   PreOps->insert(OperatorPair("<<", LEFT_SHIFT));
   PreOps->insert(OperatorPair(">>", RIGHT_SHIFT));
   PreOps->insert(OperatorPair("<", LESS));
   PreOps->insert(OperatorPair("<=", LESS_OR_EQUAL));
   PreOps->insert(OperatorPair(">", GREATER));
   PreOps->insert(OperatorPair(">=", GREATER_OR_EQUAL));
   PreOps->insert(OperatorPair("==", EQUALITY));
   PreOps->insert(OperatorPair("!=", INEQUALITY));
   PreOps->insert(OperatorPair("&", BITWISE_AND));
   PreOps->insert(OperatorPair("^", BITWISE_XOR));
   PreOps->insert(OperatorPair("|", BITWISE_OR));
   PreOps->insert(OperatorPair("&&", LOGICAL_AND));
   PreOps->insert(OperatorPair("||", LOGICAL_OR));
   PreOps->insert(OperatorPair("?", CONDITIONAL));

   Reserved.reset(new OperatorTable);
   Reserved->insert(OperatorPair(ALIGNOF_STR, ALIGNOF_TYPE));
   Reserved->insert(OperatorPair(CONST_CAST_STR, CONST_CAST));
   Reserved->insert(OperatorPair(DELETE_STR, OBJECT_DELETE));
   Reserved->insert(OperatorPair(DYNAMIC_CAST_STR, DYNAMIC_CAST));
   Reserved->insert(OperatorPair(FALSE_STR, FALSE));
   Reserved->insert(OperatorPair(NEW_STR, OBJECT_CREATE));
   Reserved->insert(OperatorPair(NOEXCEPT_STR, NOEXCEPT));
   Reserved->insert(OperatorPair(NULLPTR_STR, NULLPTR));
   Reserved->insert(OperatorPair(REINTERPRET_CAST_STR, REINTERPRET_CAST));
   Reserved->insert(OperatorPair(SIZEOF_STR, SIZEOF_TYPE));
   Reserved->insert(OperatorPair(STATIC_CAST_STR, STATIC_CAST));
   Reserved->insert(OperatorPair(THROW_STR, THROW));
   Reserved->insert(OperatorPair(TRUE_STR, TRUE));
   Reserved->insert(OperatorPair(TYPEID_STR, TYPE_NAME));

   Types.reset(new TypesTable);
   Types->insert(TypePair(AUTO_STR, AUTO_TYPE));
   Types->insert(TypePair(BOOL_STR, BOOL));
   Types->insert(TypePair(CHAR_STR, CHAR));
   Types->insert(TypePair(CHAR16_STR, CHAR16));
   Types->insert(TypePair(CHAR32_STR, CHAR32));
   Types->insert(TypePair(DOUBLE_STR, DOUBLE));
   Types->insert(TypePair(FLOAT_STR, FLOAT));
   Types->insert(TypePair(INT_STR, INT));
   Types->insert(TypePair(LONG_STR, LONG));
   Types->insert(TypePair(NULLPTR_T_STR, NULLPTR_TYPE));
   Types->insert(TypePair(SHORT_STR, SHORT));
   Types->insert(TypePair(SIGNED_STR, SIGNED));
   Types->insert(TypePair(UNSIGNED_STR, UNSIGNED));
   Types->insert(TypePair(VOID_STR, VOID));
   Types->insert(TypePair(WCHAR_STR, WCHAR));
   Types->insert(TypePair(DELETE_STR, NON_TYPE));
   Types->insert(TypePair(NEW_STR, NON_TYPE));
   Types->insert(TypePair(THROW_STR, NON_TYPE));
}

//==============================================================================

constexpr bool F = false;
constexpr bool T = true;

const CxxWord CxxWord::Attrs[Cxx::NIL_KEYWORD + 1] =
{
   //      file   class   func   advance
   CxxWord("D",   "D",    "D",   F),  // ALIGNAS
   CxxWord("@",   "@",    "@",   T),  // ASM
   CxxWord("-",   "-",    "D",   F),  // AUTO
   CxxWord("-",   "-",    "b",   T),  // BREAK
   CxxWord("-",   "-",    "c",   T),  // CASE
   CxxWord("DPC", "DPC",  "DP",  F),  // CLASS
   CxxWord("DP",  "DP",   "D",   F),  // CONST
   CxxWord("DP",  "DP",   "D",   F),  // CONSTEXPR
   CxxWord("-",   "-",    "n",   T),  // CONTINUE
   CxxWord("-",   "-",    "o",   T),  // DEFAULT
   CxxWord("-",   "-",    "d",   T),  // DO
   CxxWord("DPE", "DPE",  "DPE", F),  // ENUM
   CxxWord("-",   "P",    "-",   F),  // EXPLICIT
   CxxWord("DP",  "-",    "-",   F),  // EXTERN
   CxxWord("-",   "-",    "-",   F),  // FINAL
   CxxWord("-",   "-",    "f",   T),  // FOR
   CxxWord("-",   "F",    "-",   T),  // FRIEND
   CxxWord("-",   "-",    "g",   T),  // GOTO
   CxxWord("H",   "H",    "H",   F),  // HASH
   CxxWord("-",   "-",    "i",   T),  // IF
   CxxWord("P",   "P",    "-",   F),  // INLINE
   CxxWord("-",   "D",    "-",   F),  // MUTABLE
   CxxWord("N",   "-",    "-",   T),  // NAMESPACE
   CxxWord("-",   "P",    "-",   F),  // OPERATOR
   CxxWord("-",   "-",    "-",   F),  // OVERRIDE
   CxxWord("-",   "A",    "-",   T),  // PRIVATE
   CxxWord("-",   "A",    "-",   T),  // PROTECTED
   CxxWord("-",   "A",    "-",   T),  // PUBLIC
   CxxWord("-",   "-",    "r",   T),  // RETURN
   CxxWord("DP",  "DP",   "D",   F),  // STATIC
   CxxWord("$",   "$",    "$",   T),  // STATIC_ASSERT
   CxxWord("DPC", "DPC",  "DP",  F),  // STRUCT
   CxxWord("-",   "-",    "s",   T),  // SWITCH
   CxxWord("DCP", "DCFP", "-",   F),  // TEMPLATE
   CxxWord("D",   "D",    "D",   F),  // THREAD_LOCAL
   CxxWord("-",   "-",    "t",   T),  // TRY
   CxxWord("T",   "T",    "T",   T),  // TYPEDEF
   CxxWord("DPC", "DPC",  "DP",  F),  // UNION
   CxxWord("U",   "U",    "U",   T),  // USING
   CxxWord("-",   "P",    "-",   F),  // VIRTUAL
   CxxWord("DP",  "DP",   "D",   F),  // VOLATILE
   CxxWord("-",   "-",    "w",   T),  // WHILE
   CxxWord("-",   "P",    "-",   F),  // NVDTOR
   CxxWord("DP",  "DP",   "xD",  F)   // NIL_KEYWORD
};

//------------------------------------------------------------------------------

CxxWord::CxxWord
   (const string& file, const string& cls, const string& func, bool adv) :
   fileTarget(file),
   classTarget(cls),
   funcTarget(func),
   advance(adv)
{
   Debug::ft("CxxWord.ctor");
}

//==============================================================================

fixed_string XX = "  ";
fixed_string XN = " @";
fixed_string NX = "@ ";
fixed_string NN = "@@";
fixed_string NS = "@_";
fixed_string SN = "_@";
fixed_string SS = "__";

const CxxOp CxxOp::Attrs[Cxx::NIL_OPERATOR + 1] =
{
   //                    str arg pri ovl rl sym
   CxxOp(           SCOPE_STR, 2, 18, F, F, F, XN),  // SCOPE_RESOLUTION
   CxxOp(                 ".", 2, 17, F, F, F, NN),  // REFERENCE_SELECT
   CxxOp(                "->", 2, 17, T, F, F, NN),  // POINTER_SELECT
   CxxOp(                 "[", 2, 17, T, F, F, NN),  // ARRAY_SUBSCRIPT
   CxxOp(                 "(", 0, 17, F, F, F, NN),  // FUNCTION_CALL
   CxxOp(                "++", 1, 17, T, F, F, NX),  // POSTFIX_INCREMENT
   CxxOp(                "--", 1, 17, T, F, F, NX),  // POSTFIX_DECREMENT
   CxxOp(         DEFINED_STR, 1, 17, F, F, F, XN),  // DEFINED
   CxxOp(          TYPEID_STR, 1, 17, F, F, F, XN),  // TYPE_NAME
   CxxOp(      CONST_CAST_STR, 2, 17, F, F, F, XN),  // CONST_CAST
   CxxOp(    DYNAMIC_CAST_STR, 2, 17, F, F, F, XN),  // DYNAMIC_CAST
   CxxOp(REINTERPRET_CAST_STR, 2, 17, F, F, F, XN),  // REINTERPRET_CAST
   CxxOp(     STATIC_CAST_STR, 2, 17, F, F, F, XN),  // STATIC_CAST
   CxxOp(          SIZEOF_STR, 1, 16, F, T, F, XN),  // SIZEOF_TYPE
   CxxOp(         ALIGNOF_STR, 1, 16, F, T, F, XN),  // ALIGNOF_TYPE
   CxxOp(        NOEXCEPT_STR, 1, 16, F, T, F, XN),  // NOEXCEPT
   CxxOp(                "++", 1, 16, T, T, F, XN),  // PREFIX_INCREMENT
   CxxOp(                "--", 1, 16, T, T, F, XN),  // PREFIX_DECREMENT
   CxxOp(                 "~", 1, 16, T, T, F, XN),  // ONES_COMPLEMENT
   CxxOp(                 "!", 1, 16, T, T, F, XN),  // LOGICAL_NOT
   CxxOp(                 "+", 1, 16, T, T, F, XN),  // UNARY_PLUS
   CxxOp(                 "-", 1, 16, T, T, F, XN),  // UNARY_MINUS
   CxxOp(                 "&", 1, 16, T, T, F, XN),  // ADDRESS_OF
   CxxOp(                 "*", 1, 16, T, T, F, XN),  // INDIRECTION
   CxxOp(             NEW_STR, 0, 16, T, T, F, XX),  // OBJECT_CREATE
   CxxOp(       NEW_ARRAY_STR, 0, 16, T, T, F, XX),  // OBJECT_CREATE_ARRAY
   CxxOp(          DELETE_STR, 1, 16, T, T, F, XX),  // OBJECT_DELETE
   CxxOp(    DELETE_ARRAY_STR, 1, 16, T, T, F, XX),  // OBJECT_DELETE_ARRAY
   CxxOp(                 "(", 2, 16, T, T, F, SN),  // CAST
   CxxOp(                ".*", 2, 15, F, F, F, NN),  // REFERENCE_SELECT_MEMBER
   CxxOp(               "->*", 2, 15, T, F, F, NN),  // POINTER_SELECT_MEMBER
   CxxOp(                 "*", 2, 14, T, F, T, SS),  // MULTIPLY
   CxxOp(                 "/", 2, 14, T, F, F, SS),  // DIVIDE
   CxxOp(                 "%", 2, 14, T, F, F, SS),  // MODULO
   CxxOp(                 "+", 2, 13, T, F, T, SS),  // ADD
   CxxOp(                 "-", 2, 13, T, F, F, SS),  // SUBTRACT
   CxxOp(                "<<", 2, 12, T, F, F, SS),  // LEFT_SHIFT
   CxxOp(                ">>", 2, 12, T, F, F, SS),  // RIGHT_SHIFT
   CxxOp(                 "<", 2, 11, T, F, T, SS),  // LESS
   CxxOp(                "<=", 2, 11, T, F, T, SS),  // LESS_OR_EQUAL
   CxxOp(                 ">", 2, 11, T, F, T, SS),  // GREATER
   CxxOp(                ">=", 2, 11, T, F, T, SS),  // GREATER_OR_EQUAL
   CxxOp(                "==", 2, 10, T, F, T, SS),  // EQUALITY
   CxxOp(                "!=", 2, 10, T, F, T, SS),  // INEQUALITY
   CxxOp(                 "&", 2,  9, T, F, T, SS),  // BITWISE_AND
   CxxOp(                 "^", 2,  8, T, F, T, SS),  // BITWISE_XOR
   CxxOp(                 "|", 2,  7, T, F, T, SS),  // BITWISE_OR
   CxxOp(                "&&", 2,  6, T, F, T, SS),  // LOGICAL_AND
   CxxOp(                "||", 2,  5, T, F, T, SS),  // LOGICAL_OR
   CxxOp(                 "?", 3,  4, F, F, F, SS),  // CONDITIONAL
   CxxOp(                 "=", 2,  3, T, T, F, SS),  // ASSIGN
   CxxOp(                "*=", 2,  3, T, T, F, SS),  // MULTIPLY_ASSIGN
   CxxOp(                "/=", 2,  3, T, T, F, SS),  // DIVIDE_ASSIGN
   CxxOp(                "%=", 2,  3, T, T, F, SS),  // MODULO_ASSIGN
   CxxOp(                "+=", 2,  3, T, T, F, SS),  // ADD_ASSIGN
   CxxOp(                "-=", 2,  3, T, T, F, SS),  // SUBTRACT_ASSIGN
   CxxOp(               "<<=", 2,  3, T, T, F, SS),  // LEFT_SHIFT_ASSIGN
   CxxOp(               ">>=", 2,  3, T, T, F, SS),  // RIGHT_SHIFT_ASSIGN
   CxxOp(                "&=", 2,  3, T, T, F, SS),  // BITWISE_AND_ASSIGN
   CxxOp(                "^=", 2,  3, T, T, F, SS),  // BITWISE_XOR_ASSIGN
   CxxOp(                "|=", 2,  3, T, T, F, SS),  // BITWISE_OR_ASSIGN
   CxxOp(           THROW_STR, 0,  2, F, T, F, SS),  // THROW
   CxxOp(                 ",", 2,  1, F, F, F, NS),  // STATEMENT_SEPARATOR
   CxxOp(                 "$", 0,  0, F, F, F, XX),  // START_OF_EXPRESSION
   CxxOp(           ERROR_STR, 0,  0, F, F, F, XX),  // FALSE
   CxxOp(           ERROR_STR, 0,  0, F, F, F, XX),  // TRUE
   CxxOp(           ERROR_STR, 0,  0, F, F, F, XX),  // NULLPTR
   CxxOp(           ERROR_STR, 0,  0, F, F, F, XX)   // NIL_OPERATOR
};

//------------------------------------------------------------------------------

CxxOp::CxxOp(const string& sym, size_t args,
   size_t prio, bool over, bool push, bool symm, c_string sp) :
   symbol(sym),
   arguments(args),
   priority(prio),
   overloadable(over),
   rightToLeft(push),
   symmetric(symm),
   spacing(sp)
{
   Debug::ft("CxxOp.ctor");
}

//------------------------------------------------------------------------------

Cxx::Operator CxxOp::NameToOperator(const string& name)
{
   Debug::ft("CxxOp.NameToOperator");

   auto pos = name.rfind(OPERATOR_STR);
   if(pos == string::npos) return Cxx::NIL_OPERATOR;

   auto sym = name.substr(pos + strlen(OPERATOR_STR));
   pos = sym.find_first_not_of(SPACE);
   if(pos == string::npos) return Cxx::CAST;
   if(pos > 0) sym.erase(0, pos);

   auto last = sym.back();
   if((last == ')') || (last == ']')) sym.pop_back();

   for(size_t i = 0; i <= Cxx::STATEMENT_SEPARATOR; ++i)
   {
      if(Attrs[i].symbol.compare(sym) == 0) return Cxx::Operator(i);
   }

   return Cxx::NIL_OPERATOR;
}

//------------------------------------------------------------------------------

string CxxOp::OperatorToName(Cxx::Operator oper)
{
   Debug::ft("CxxOp.OperatorToName");

   const auto& attrs = Attrs[oper];
   string name(OPERATOR_STR);
   if(isalpha(attrs.symbol.front())) name += SPACE;
   name += attrs.symbol;

   switch(oper)
   {
   case Cxx::ARRAY_SUBSCRIPT:
      name += ']';
      break;
   case Cxx::FUNCTION_CALL:
   case Cxx::CAST:
      name += ')';
   }

   return name;
}

//------------------------------------------------------------------------------

void CxxOp::UpdateOperator(Cxx::Operator& oper, size_t args)
{
   Debug::ft("CxxOp.UpdateOperator");

   const auto& attrs = Attrs[oper];

   if((attrs.arguments == args) || (attrs.arguments == 0)) return;

   const auto& token = attrs.symbol;

   for(size_t i = 0; i <= Cxx::STATEMENT_SEPARATOR; ++i)
   {
      const auto& entry = Attrs[i];

      if((entry.arguments == args) && (entry.symbol.compare(token) == 0))
      {
         oper = Cxx::Operator(i);
         return;
      }
   }
}

//==============================================================================

CxxChar CxxChar::Attrs[UINT8_MAX + 1] = { };

//------------------------------------------------------------------------------

void CxxChar::Initialize()
{
   Debug::ft("CxxChar.Initialize");

   for(auto c = 0; c <= UINT8_MAX; ++c)
   {
      Attrs[c].validFirst = false;
      Attrs[c].validNext = false;
      Attrs[c].validOp = false;
      Attrs[c].validInt = false;
      Attrs[c].intValue = -1;
      Attrs[c].hexValue = -1;
      Attrs[c].octValue = -1;
   }

   for(size_t i = 0; i < ValidFirstChars.size(); ++i)
   {
      Attrs[ValidFirstChars[i]].validFirst = true;
   }

   for(size_t i = 0; i < ValidNextChars.size(); ++i)
   {
      Attrs[ValidNextChars[i]].validNext = true;
   }

   for(size_t i = 0; i < ValidOpChars.size(); ++i)
   {
      Attrs[ValidOpChars[i]].validOp = true;
   }

   for(size_t i = 0; i < ValidIntChars.size(); ++i)
   {
      Attrs[ValidIntChars[i]].validInt = true;
   }

   for(size_t i = 0; i < ValidIntDigits.size(); ++i)
   {
      Attrs[ValidIntDigits[i]].intValue = int8_t(i);
   }

   for(size_t i = 0; i < ValidHexDigits.size(); ++i)
   {
      auto h = (i <= 15 ? i : i - 6);
      Attrs[ValidHexDigits[i]].hexValue = int8_t(h);
   }

   for(size_t i = 0; i < ValidOctDigits.size(); ++i)
   {
      Attrs[ValidOctDigits[i]].octValue = int8_t(i);
   }
}

//==============================================================================

const Numeric Numeric::Nil(NIL, 0, F);
const Numeric Numeric::Bool(INT, 1, F);
const Numeric Numeric::Char(INT, sizeof(char) << 3, T);
const Numeric Numeric::Char16(INT, sizeof(char16_t) << 3, F);
const Numeric Numeric::Char32(INT, sizeof(char32_t) << 3, F);
const Numeric Numeric::Double(FLOAT, sizeof(double) << 3, T);
const Numeric Numeric::Enum(ENUM, sizeof(int) << 3, T);
const Numeric Numeric::Float(FLOAT, sizeof(float) << 3, T);
const Numeric Numeric::Int(INT, sizeof(int) << 3, T);
const Numeric Numeric::Long(INT, sizeof(long) << 3, T);
const Numeric Numeric::LongDouble(FLOAT, sizeof(long double) << 3, T);
const Numeric Numeric::LongLong(INT, sizeof(long long) << 3, T);
const Numeric Numeric::Pointer(PTR, sizeof(intptr_t), T);
const Numeric Numeric::Short(INT, sizeof(short) << 3, T);
const Numeric Numeric::uChar(INT, sizeof(char) << 3, F);
const Numeric Numeric::uInt(INT, sizeof(int) << 3, F);
const Numeric Numeric::uLong(INT, sizeof(long) << 3, F);
const Numeric Numeric::uLongLong(INT, sizeof(long long) << 3, F);
const Numeric Numeric::uShort(INT, sizeof(short) << 3, F);
const Numeric Numeric::wChar(INT, sizeof(wchar_t) << 3, F);

//------------------------------------------------------------------------------

TypeMatch Numeric::CalcMatchWith(const Numeric* that) const
{
   Debug::ft("Numeric.CalcMatchWith");

   //  Determine whether THAT can be implicitly converted to THIS.
   //
   switch(this->type_)
   {
   case INT:
      switch(that->type_)
      {
      case INT:
      case ENUM:
         if(this->bitWidth_ == that->bitWidth_)
         {
            if(this->signed_ != that->signed_) return Convertible;
            if(that->type_ == ENUM) return Promotable;
            return Compatible;
         }
         else if(this->bitWidth_ > that->bitWidth_)
         {
            if(that->signed_ && !this->signed_) return Convertible;
            return Promotable;
         }
         return Abridgeable;

      case PTR:
         if(this->bitWidth_ >= that->bitWidth_) return Convertible;
         return Abridgeable;

      case FLOAT:
         return Abridgeable;
      }

      return Incompatible;

   case FLOAT:
      if(that->type_ == FLOAT) return Convertible;
      if(that->type_ == INT) return Convertible;
      return Incompatible;

   case PTR:
      if(that->type_ != INT) return Incompatible;
      if(this->bitWidth_ != that->bitWidth_) return Incompatible;
      return Convertible;
   }

   return Incompatible;
}

//==============================================================================

SymbolView::SymbolView() :
   accessibility_(Inaccessible),
   control_(Cxx::Access_N),
   match_(Compatible),
   defts_(false),
   using_(false),
   friend_(false),
   resolved_(false),
   distance_(0)
{
}

//------------------------------------------------------------------------------

SymbolView::SymbolView(Accessibility a, TypeMatch m,
   Cxx::Access c, bool t, bool u, bool f, bool r, Distance d) :
   accessibility_(a),
   control_(c),
   match_(m),
   defts_(t),
   using_(u),
   friend_(f),
   resolved_(r),
   distance_(d)
{
}

//------------------------------------------------------------------------------

const SymbolView NotAccessible
   (Inaccessible, Compatible, Cxx::Access_N, false, false, false, true, 0);
const SymbolView DeclaredGlobally
   (Unrestricted, Compatible, Cxx::Access_N, false, false, false, true, 0);
const SymbolView DeclaredLocally
   (Declared, Compatible, Cxx::Access_N, false, false, false, true, 0);

//==============================================================================
//
//  Removes, from SET, an item that is
//    (a) a template parameter,
//    (b) a template argument in TYPE, or
//    (c) a name found in NAMES.
//  There are situation in which (b) or (c), but not both, detects a template
//  argument.  Ideally this would be cleaned up, but the effort does not seem
//  worthwhile.
//
static void EraseTemplateArgs
   (CxxNamedSet& set, const TypeName* type, const stringVector& names)
{
   for(auto i = set.cbegin(); i != set.cend(); NO_OP)
   {
      auto name = (*i)->ScopedName(true);
      auto erase = ((*i)->Type() == Cxx::TemplateParm);
      erase = erase || type->ItemIsTemplateArg(*i);

      if(!erase)
      {
         for(auto n = names.cbegin(); n != names.cend(); ++n)
         {
            if(name == *n)
            {
               erase = true;
               break;
            }
         }
      }

      if(erase)
         i = set.erase(i);
      else
         ++i;
   }
}

//------------------------------------------------------------------------------
//
//  LHS = LHS U RHS.
//
static void Union(CxxNamedSet& lhs, const CxxNamedSet& rhs)
{
   for(auto i = rhs.cbegin(); i != rhs.cend(); ++i)
   {
      lhs.insert(*i);
   }
}

//------------------------------------------------------------------------------

void CxxUsageSets::AddBase(CxxNamed* item)
{
   if(item->GetFile() == nullptr) return;
   bases.insert(item);
}

//------------------------------------------------------------------------------

void CxxUsageSets::AddDirect(CxxNamed* item)
{
   if(item->GetFile() == nullptr) return;
   directs.insert(item);
}

//------------------------------------------------------------------------------

void CxxUsageSets::AddForward(CxxNamed* item)
{
   if(item->GetFile() == nullptr) return;
   if(item->Type() == Cxx::Friend)
      friends.insert(item);
   else
      forwards.insert(item);
}

//------------------------------------------------------------------------------

void CxxUsageSets::AddIndirect(CxxNamed* item)
{
   if(item->GetFile() == nullptr) return;
   indirects.insert(item);
}

//------------------------------------------------------------------------------

void CxxUsageSets::AddInherit(CxxNamed* item)
{
   if(item->GetFile() == nullptr) return;
   inherits.insert(item);
}

//------------------------------------------------------------------------------

void CxxUsageSets::AddUser(CxxNamed* item)
{
   if(item->GetFile() == nullptr) return;
   users.insert(item);
}

//------------------------------------------------------------------------------

void CxxUsageSets::EraseLocals()
{
   Debug::ft("CxxUsageSets.EraseLocals");

   for(auto d = directs.cbegin(); d != directs.cend(); NO_OP)
   {
      if((*d)->ScopedName(false).find(LOCALS_STR) != string::npos)
         d = directs.erase(d);
      else
         ++d;
   }
}

//------------------------------------------------------------------------------

void CxxUsageSets::EraseTemplateArgs(const TypeName* type)
{
   Debug::ft("CxxUsageSets.EraseTemplateArgs");

   stringVector names;
   type->GetNames(names);
   CodeTools::EraseTemplateArgs(directs, type, names);
   CodeTools::EraseTemplateArgs(indirects, type, names);
   CodeTools::EraseTemplateArgs(forwards, type, names);
}

//------------------------------------------------------------------------------

void CxxUsageSets::Union(const CxxUsageSets& set)
{
   Debug::ft("CxxUsageSets.Union");

   CodeTools::Union(bases, set.bases);
   CodeTools::Union(directs, set.directs);
   CodeTools::Union(indirects, set.indirects);
   CodeTools::Union(forwards, set.forwards);
   CodeTools::Union(friends, set.friends);
   CodeTools::Union(users, set.users);
}
}
