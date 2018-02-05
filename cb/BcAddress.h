//==============================================================================
//
//  BcAddress.h
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
#ifndef BCADDRESS_H_INCLUDED
#define BCADDRESS_H_INCLUDED

#include "Protected.h"
#include "TlvParameter.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "SbTypes.h"
#include "SysTypes.h"

using namespace SessionBase;

//------------------------------------------------------------------------------

namespace CallBase
{
//  Definitions for digits.
//
//  Digit_0 is encoded as 10 because, historically, it was transmitted as ten
//  signal breaks on a rotary phone.  Digit_Star and Digit_Hash correspond to
//  the '*' and '#' keys on the typical keypad.  There are actually 16 DTMF
//  digits in all, but public networks do not use 0 and 13-15.
//
enum Digit
{
   NilDigit,
   Digit_1,
   Digit_2,
   Digit_3,
   Digit_4,
   Digit_5,
   Digit_6,
   Digit_7,
   Digit_8,
   Digit_9,
   Digit_0,
   Digit_Star,
   Digit_Hash
};

//------------------------------------------------------------------------------
//
//  A network defines addresses to identify subscribers (users) and services.
//
class Address : public Protected
{
public:
   //  Currently two types of addresses are supported: directory numbers
   //  and service codes.
   //
   enum Type
   {
      Invalid,  // invalid address
      DnType,   // director number
      ScType,   // service code (preceded by '*')
      Type_N    // number of address types
   };

   //  Type for a directory number.  Valid DNs are 20000 to 99999.
   //
   typedef uint32_t DN;

   static const DN NilDN   = 0;
   static const DN FirstDN = 20000;
   static const DN LastDN  = 99999;

   //  The length of a directory number.
   //
   static const size_t DN_Length = 5;

   //  Virtual to allow subclassing.
   //
   virtual ~Address();

   //  Returns true if DN is a valid directory number.
   //
   static bool IsValidDN(DN dn) { return ((dn >= FirstDN) && (dn <= LastDN)); }

   //  Type for a service code.  Valid SCs are *20 to *99.
   //
   typedef uint32_t SC;

   static const SC NilSC   = 0;
   static const SC FirstSC = 20;
   static const SC LastSC  = 99;

   //  The length of a service code.
   //
   static const size_t SC_Length = 3;

   //  Returns true if SC is a valid service code.
   //
   static bool IsValidSC(SC sc) { return ((sc >= FirstSC) && (sc <= LastSC)); }

   //  Maps a directory number to an index, with the first valid DN being 1.
   //
   static uint32_t DNToIndex(Address::DN dn) { return dn - FirstDN + 1; }

   //  Maps an index to a directory number, with 1 being the first valid DN.
   //
   static Address::DN IndexToDN(uint32_t i) { return i + FirstDN - 1; }
protected:
   //  Protected because this class is virtual.
   //
   Address();
};

//  Inserts a string for TYPE into STREAM.
//
std::ostream& operator<<(std::ostream& stream, Address::Type type);

//------------------------------------------------------------------------------
//
//  A digit string typically serves to specify an Address.  Digit strings are
//  included in protocols, so this class (struct) must not be subclassed.
//
class DigitString
{
public:
   //  The number of digits in a string.
   //
   typedef uint8_t DigitCount;

   //  The maximum number of digits allowed in a string.
   //
   static const DigitCount MaxDigitCount = 8;

   //  Outcomes when building a digit string.
   //
   enum Rc
   {
      Ok,            // no errors encountered
      Complete,      // string is complete (terminated by '#')
      IllegalDigit,  // tried to add an illegal digit
      Overflow       // tried to add too many digits
   };

   //  Creates an empty string.
   //
   DigitString();

   //  Creates a string that corresponds to DN.  Deliberately implicit.
   //
   DigitString(Address::DN dn);

   //  Creates a string that corresponds to S.
   //
   explicit DigitString(const std::string& s);

   //  Returns true if D is a valid digit.
   //
   static bool IsValidDigit(Digit d)
   {
      return ((d >= Digit_1) && (d <= Digit_Hash));
   }

   //  Adds D to the string.
   //
   Rc AddDigit(Digit d);

   //  Adds S to the string.  S may contain the characters 0-9, *, and #.
   //
   Rc AddDigits(const std::string& s);

   //  Adds DS to the string.
   //
   Rc AddDigits(const DigitString& ds);

   //  Returns true if the string is complete.  This either means that
   //  o the string maps to a valid address, or
   //  o the string does not map to a valid address and adding more digits
   //    will not change this.
   //
   bool IsCompleteAddress() const;

   //  Returns the number of digits in the string.
   //
   DigitCount Size() const;

   //  Returns true if the string contains no digits.
   //
   bool Empty() const { return (Size() == 0); }

   //  Resets the string to empty.
   //
   void Clear();

   //  Returns the digit at position I.
   //
   Digit GetDigit(DigitCount i) const;

   //  Converts the string to an address.
   //
   Address::DN ToDN() const;

   //  Converts the string to a service code.
   //
   Address::SC ToSC() const;

   //  Returns true if two digit strings are identical.
   //
   bool operator==(const DigitString& that) const;

   //  Returns true if two digit strings are different.
   //
   bool operator!=(const DigitString& that) const;

   //  Displays member variables, similar to Base::Display.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;
private:
   //  The string of digits, stored as an array.
   //
   uint8_t digits_[MaxDigitCount];

   //  The number of digits in the string.
   //
   DigitCount size_;

   //  DigitToChar[d] maps Digit d (0-15) to a character (0-9, *, or #,
   //  with ? used for an illegal digit).
   //
   static fixed_string DigitToChar;
};

//------------------------------------------------------------------------------
//
//  Virtual base class for supporting a DigitString parameter.
//
class AddressParameter : public TlvParameter
{
protected:
   //  Protected because this class is virtual.
   //
   AddressParameter(ProtocolId prid, Id pid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~AddressParameter();

   //  Overridden to invoke DigitString::Display.
   //
   virtual void DisplayMsg(std::ostream& stream, const std::string& prefix,
      const byte_t* bytes, size_t count) const override;

   //  Overridden to add a DigitString to MSG.
   //
   virtual TestRc InjectMsg
      (CliThread& cli, Message& msg, Usage use) const override;

   //  Overridden to check a DigitString in MSG against an expected value.
   //
   virtual TestRc VerifyMsg
      (CliThread& cli, const Message& msg, Usage use) const override;
};
}
#endif
