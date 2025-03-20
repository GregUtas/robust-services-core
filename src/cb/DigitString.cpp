//==============================================================================
//
//  DigitString.cpp
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
#include "BcAddress.h"
#include <ostream>
#include "Debug.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CallBase
{
//  DigitToChar[d] maps Digit d (0-15) to a character (0-9, *, or #,
//  with ? used for an illegal digit).
//
static fixed_string DigitToChar = "?1234567890*#???";

//------------------------------------------------------------------------------

DigitString::DigitString() :
   digits_{0},
   size_(0)
{
   Debug::ft("DigitString.ctor");
}

//------------------------------------------------------------------------------

DigitString::DigitString(Address::DN dn) :
   digits_{0},
   size_(0)
{
   Debug::ft("DigitString.ctor(dn)");

   if(Address::IsValidDN(dn))
   {
      for(size_t i = 1; i <= Address::DN_Length; ++i)
      {
         auto d = dn % 10;
         if(d == 0) d = Digit_0;
         digits_[Address::DN_Length - i] = d;
         dn /= 10;
      }

      size_ = Address::DN_Length;
   }
}

//------------------------------------------------------------------------------

DigitString::DigitString(const string& s) :
   digits_{0},
   size_(0)
{
   Debug::ft("DigitString.ctor(string)");

   AddDigits(s);
}

//------------------------------------------------------------------------------

DigitString::Rc DigitString::AddDigit(Digit d)
{
   Debug::ft("DigitString.AddDigit");

   if((size_ > 0) && (digits_[size_ - 1] == Digit_Hash)) return Complete;
   if(!IsValidDigit(d)) return IllegalDigit;
   if(size_ >= MaxDigitCount) return Overflow;
   digits_[size_++] = d;
   if(d == Digit_Hash) return Complete;
   return Ok;
}

//------------------------------------------------------------------------------

DigitString::Rc DigitString::AddDigits(const string& s)
{
   Debug::ft("DigitString.AddDigits(string)");

   Rc rc;
   auto len = s.size();

   for(size_t i = 0; i < len; ++i)
   {
      switch(s[i])
      {
      case '1': rc = AddDigit(Digit_1); break;
      case '2': rc = AddDigit(Digit_2); break;
      case '3': rc = AddDigit(Digit_3); break;
      case '4': rc = AddDigit(Digit_4); break;
      case '5': rc = AddDigit(Digit_5); break;
      case '6': rc = AddDigit(Digit_6); break;
      case '7': rc = AddDigit(Digit_7); break;
      case '8': rc = AddDigit(Digit_8); break;
      case '9': rc = AddDigit(Digit_9); break;
      case '0': rc = AddDigit(Digit_0); break;
      case '*': rc = AddDigit(Digit_Star); break;
      case '#': rc = AddDigit(Digit_Hash); break;
      default: return IllegalDigit;
      }

      if(rc != Ok) return rc;
   }

   return Ok;
}

//------------------------------------------------------------------------------

DigitString::Rc DigitString::AddDigits(const DigitString& ds)
{
   Debug::ft("DigitString.AddDigits(digits)");

   for(size_t i = 0; i < ds.size_; ++i)
   {
      auto rc = AddDigit(Digit(ds.digits_[i]));
      if(rc != Ok) return rc;
   }

   return Ok;
}

//------------------------------------------------------------------------------

Digit DigitString::At(DigitCount i) const
{
   if(i < Size()) return Digit(digits_[i]);
   return NilDigit;
}

//------------------------------------------------------------------------------

void DigitString::Clear()
{
   Debug::ft("DigitString.Clear");

   size_ = 0;
}

//------------------------------------------------------------------------------

void DigitString::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << "count  : " << int(size_) << CRLF;
   stream << prefix << "digits : ";
   for(size_t i = 0; i < size_; ++i) stream << DigitToChar[digits_[i]];
   stream << CRLF;
}

//------------------------------------------------------------------------------

fn_name DigitString_IsCompleteAddress = "DigitString.IsCompleteAddress";

bool DigitString::IsCompleteAddress() const
{
   Debug::ft(DigitString_IsCompleteAddress);

   if(size_ == 0) return false;
   if(digits_[size_ - 1] == Digit_Hash) return true;

   switch(digits_[0])
   {
   case Digit_Star:
      return (Size() >= Address::SC_Length);

   case Digit_0:
   case Digit_1:
      return true;

   case NilDigit:
   case Digit_Hash:
      Debug::SwLog(DigitString_IsCompleteAddress, "invalid digit", digits_[0]);
      return true;
   }

   return (Size() >= Address::DN_Length);
}

//------------------------------------------------------------------------------

bool DigitString::operator==(const DigitString& that) const
{
   if(size_ != that.size_) return false;

   for(size_t i = 0; i < size_; ++i)
   {
      if(digits_[i] != that.digits_[i]) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

bool DigitString::operator!=(const DigitString& that) const
{
   return !(*this == that);
}

//------------------------------------------------------------------------------

DigitString::DigitCount DigitString::Size() const
{
   Debug::ft("DigitString.Size");

   if(size_ == 0) return 0;
   if(digits_[size_ - 1] == Digit_Hash) return size_ - 1;
   return size_;
}

//------------------------------------------------------------------------------

Address::DN DigitString::ToDN() const
{
   Debug::ft("DigitString.ToDN");

   if(Size() == Address::DN_Length)
   {
      Address::DN dn = 0;

      for(size_t i = 0; i < Address::DN_Length; ++i)
      {
         auto d = digits_[i];
         if((d < Digit_1) || (d > Digit_0)) return Address::NilDN;
         if(d == Digit_0) d = 0;
         dn = (dn * 10) + d;
      }

      if(!Address::IsValidDN(dn)) return Address::NilDN;
      return dn;
   }

   return Address::NilDN;
}

//------------------------------------------------------------------------------

Address::SC DigitString::ToSC() const
{
   Debug::ft("DigitString.ToSC");

   if(Size() == Address::SC_Length)
   {
      Address::SC sc = 0;

      if(digits_[0] != Digit_Star) return Address::NilSC;

      for(size_t i = 1; i < Address::SC_Length; ++i)
      {
         auto d = digits_[i];
         if((d < Digit_1) || (d > Digit_0)) return Address::NilSC;
         if(d == Digit_0) d = 0;
         sc = (sc * 10) + d;
      }

      if(!Address::IsValidSC(sc)) return Address::NilSC;
      return sc;
   }

   return Address::NilSC;
}
}
