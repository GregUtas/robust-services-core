//==============================================================================
//
//  CxxStrLiteral.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef CXXSTRLITERAL_H_INCLUDED
#define CXXSTRLITERAL_H_INCLUDED

#include "CxxToken.h"
#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include "Cxx.h"
#include "CxxArea.h"
#include "CxxCharLiteral.h"
#include "CxxFwd.h"
#include "CxxNamed.h"
#include "CxxRoot.h"
#include "CxxScope.h"
#include "Debug.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace CodeTools
{
//  A string literal ("s", u"s", u8"s", U"s", or L"s").
//
template<typename C, class T, Cxx::Encoding E>
   class CxxStrLiteral : public StringLiteral
{
public:
   CxxStrLiteral() = default;

   explicit CxxStrLiteral(const T& s) : str_(s) { }

   ~CxxStrLiteral() = default;

   CxxStrLiteral(const CxxStrLiteral& that) = delete;

   CxxStrLiteral& operator=(const CxxStrLiteral& that) = delete;

   const T& GetStr() const { return str_; }

   void Replace(const T& str) { str_ = str; }

   static CxxScoped* GetReferent()
   {
      Debug::ft(CxxStrLiteral_GetReferent);
      return Ref_[E].get();
   }

   //  Creates a data item whose type is "const E* const".  A FuncData
   //  instance is created because SpaceData would try to open a scope
   //  in the parser's current scope, which doesn't exist when this is
   //  invoked during system initialization.
   //
   static DataPtr CreateRef()
   {
      Debug::ft(CxxStrLiteral_CreateRef);

      DataPtr data;
      auto ctype = CxxCharLiteral<C, E>::TypeStr();
      std::string dataName("__string<");
      dataName.append(ctype);
      dataName.append(">_literal_referent");
      QualNamePtr typeName(new QualName(ctype));
      TypeSpecPtr typeSpec(new DataSpec(typeName));
      typeSpec->Tags()->SetConst(true);
      typeSpec->Tags()->SetPointer(0, true, false);
      data.reset(new FuncData(dataName, typeSpec));
      data->SetScope(Singleton<CxxRoot>::Instance()->GlobalNamespace());
      return data;
   }

   void PushBack(uint32_t c) override { str_.push_back(c); }
private:
   //  Function names.
   //
   inline static fn_name CxxStrLiteral_CreateRef = "CxxStrLiteral.CreateRef";
   inline static fn_name
      CxxStrLiteral_GetReferent = "CxxStrLiteral.GetReferent";

   Numeric GetNumeric() const override { return Numeric::Pointer; }

   TypeSpec* GetTypeSpec() const override { return Ref_[E]->GetTypeSpec(); }

   void Print(std::ostream& stream, const Flags& options) const override
   {
      stream << E << QUOTE;
      for(size_t i = 0; i < str_.size(); ++i)
         stream << CharString(str_[i], true);
      stream << QUOTE;
   }

   CxxScoped* Referent() const override { return GetReferent(); }

   std::string TypeString(bool arg) const override
   {
      auto type = CxxCharLiteral<C, E>::TypeStr();
      type.push_back('*');
      return type;
   }

   //  A string for holding the string literal.
   //
   T str_;

   //  The underlying type for each string literal (e.g. const char* const).
   //
   static const DataPtr Ref_[Cxx::Encoding_N];
};

using StringLiteralPtr = std::unique_ptr<StringLiteral>;
using StrLiteral = CxxStrLiteral<char, std::string, Cxx::ASCII>;
using u8StrLiteral = CxxStrLiteral<char, std::string, Cxx::ASCII>;
using u16StrLiteral = CxxStrLiteral<char16_t, std::u16string, Cxx::U16>;
using u32StrLiteral = CxxStrLiteral<char32_t, std::u32string, Cxx::U32>;
using wStrLiteral = CxxStrLiteral<wchar_t, std::wstring, Cxx::WIDE>;

template<typename C, class T, Cxx::Encoding E>
   const DataPtr CxxStrLiteral<C, T, E>::Ref_[Cxx::Encoding_N] =
{
   StrLiteral::CreateRef(),
   u8StrLiteral::CreateRef(),
   u16StrLiteral::CreateRef(),
   u32StrLiteral::CreateRef(),
   wStrLiteral::CreateRef()
};
}
#endif
