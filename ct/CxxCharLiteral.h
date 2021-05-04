//==============================================================================
//
//  CxxCharLiteral.h
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
#ifndef CXXCHARLITERAL_H_INCLUDED
#define CXXCHARLITERAL_H_INCLUDED

#include "CxxToken.h"
#include <ostream>
#include <string>
#include "CodeTypes.h"
#include "Cxx.h"
#include "CxxRoot.h"
#include "CxxScoped.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A character literal ('c', u'c', U'c', or L'c').  Note that u8'c' is C++17
//  and that u8"s" (a string literal) is of type const char* const in C++11.
//
template< typename C, Cxx::Encoding E > class CxxCharLiteral : public Literal
{
public:
   explicit CxxCharLiteral(C c) : c_(c)
   {
      CxxStats::Incr(CxxStats::CHAR_LITERAL);
   }

   ~CxxCharLiteral()
   {
      CxxStats::Decr(CxxStats::CHAR_LITERAL);
   }

   CxxCharLiteral(const CxxCharLiteral& that) = delete;

   CxxCharLiteral& operator=(const CxxCharLiteral& that) = delete;

   void Print(std::ostream& stream, const Flags& options) const override
   {
      stream << E << APOSTROPHE << CharString(c_, false) << APOSTROPHE;
   }

   static std::string TypeStr()
   {
      switch(E)
      {
      case Cxx::ASCII: return CHAR_STR;
      case Cxx::U8: return CHAR_STR;
      case Cxx::U16: return CHAR16_STR;
      case Cxx::U32: return CHAR32_STR;
      case Cxx::WIDE: return WCHAR_STR;
      }
      return ERROR_STR;
   }
private:
   CxxScoped* Referent() const override
   {
      switch(E)
      {
      case Cxx::ASCII: return Singleton< CxxRoot >::Instance()->CharTerm();
      case Cxx::U8: return Singleton< CxxRoot >::Instance()->CharTerm();
      case Cxx::U16: return Singleton< CxxRoot >::Instance()->Char16Term();
      case Cxx::U32: return Singleton< CxxRoot >::Instance()->Char32Term();
      case Cxx::WIDE: return Singleton< CxxRoot >::Instance()->wCharTerm();
      }
      return nullptr;
   }

   std::string TypeString(bool arg) const override { return TypeStr(); }

   Numeric GetNumeric() const override
   {
      switch(E)
      {
      case Cxx::ASCII: return Numeric::Char;
      case Cxx::U8: return Numeric::Char;
      case Cxx::U16: return Numeric::Char16;
      case Cxx::U32: return Numeric::Char32;
      case Cxx::WIDE: return Numeric::wChar;
      }
      return Numeric::Nil;
   }

   //  The character that appeared in the literal.
   //
   const C c_;
};

using CharLiteral = CxxCharLiteral<char, Cxx::ASCII>;
using u16CharLiteral = CxxCharLiteral<char16_t, Cxx::U16>;
using u32CharLiteral = CxxCharLiteral<char32_t, Cxx::U32>;
using wCharLiteral = CxxCharLiteral<wchar_t, Cxx::WIDE>;
}
#endif
