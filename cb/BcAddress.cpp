//==============================================================================
//
//  BcAddress.cpp
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
#include "BcAddress.h"
#include <sstream>
#include "CliCommand.h"
#include "CliThread.h"
#include "Debug.h"
#include "Formatters.h"
#include "SbCliParms.h"
#include "TlvMessage.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CallBase
{
fn_name Address_ctor = "Address.ctor";

Address::Address()
{
   Debug::ft(Address_ctor);
}

//------------------------------------------------------------------------------

fn_name Address_dtor = "Address.dtor";

Address::~Address()
{
   Debug::ft(Address_dtor);
}

//------------------------------------------------------------------------------

fixed_string AddressTypeStrings[Address::Type_N + 1] =
{
   "Invalid",
   "Directory Number",
   "Service Code",
   ERROR_STR
};

ostream& operator<<(ostream& stream, Address::Type type)
{
   if((type >= 0) && (type < Address::Type_N))
      stream << AddressTypeStrings[type];
   else
      stream << AddressTypeStrings[Address::Type_N];
   return stream;
}

//==============================================================================

fn_name AddressParameter_ctor = "AddressParameter.ctor";

AddressParameter::AddressParameter(ProtocolId prid, Id pid) :
   TlvParameter(prid, pid)
{
   Debug::ft(AddressParameter_ctor);
}

//------------------------------------------------------------------------------

fn_name AddressParameter_dtor = "AddressParameter.dtor";

AddressParameter::~AddressParameter()
{
   Debug::ft(AddressParameter_dtor);
}

//------------------------------------------------------------------------------

void AddressParameter::DisplayMsg(ostream& stream,
   const string& prefix, const byte_t* bytes, size_t count) const
{
   reinterpret_cast< const DigitString* >(bytes)->Display(stream, prefix);
}

//------------------------------------------------------------------------------

fixed_string IllegalDigitExpl = "Illegal digit in digit string.";
fixed_string TooManyDigitsExpl = "The digit string is too long.";

fn_name AddressParameter_InjectMsg = "AddressParameter.InjectMsg";

Parameter::TestRc AddressParameter::InjectMsg
   (CliThread& cli, Message& msg, Usage use) const
{
   Debug::ft(AddressParameter_InjectMsg);

   string digits;
   DigitString ds = DigitString();
   auto& tlvmsg = static_cast< TlvMessage& >(msg);

   switch(cli.Command()->GetStringRc(digits, cli))
   {
   case CliParm::None:
      if(use == Mandatory) return StreamMissingMandatoryParm;
      return Ok;
      break;
   case CliParm::Ok:
      break;
   default:
      return IllegalValueInStream;
   }

   auto rc = ds.AddDigits(digits);

   switch(rc)
   {
   case DigitString::Ok:
   case DigitString::Complete:
      break;
   case DigitString::IllegalDigit:
      *cli.obuf << spaces(2) << IllegalDigitExpl << CRLF;
      return IllegalValueInStream;
   case DigitString::Overflow:
      *cli.obuf << spaces(2) << TooManyDigitsExpl << CRLF;
      return IllegalValueInStream;
   default:
      Debug::SwErr(AddressParameter_InjectMsg, rc, 0);
      return IllegalValueInStream;
   }

   if(tlvmsg.AddType(ds, Pid()) == nullptr)
   {
      *cli.obuf << ParameterNotAdded << CRLF;
      return MessageFailedToAddParm;
   }

   return Ok;
}

//------------------------------------------------------------------------------

fn_name AddressParameter_VerifyMsg = "AddressParameter.VerifyMsg";

Parameter::TestRc AddressParameter::VerifyMsg
   (CliThread& cli, const Message& msg, Usage use) const
{
   Debug::ft(AddressParameter_VerifyMsg);

   TestRc rc;
   auto& tlvmsg = static_cast< const TlvMessage& >(msg);
   DigitString* info;
   string digits;
   DigitString ds = DigitString();

   rc = tlvmsg.VerifyParm(Pid(), use, info);
   if(rc != Ok) return rc;
   if(use == Illegal) return Ok;

   //  Get the digit string supplied by InjectCommand.
   //
   switch(cli.Command()->GetStringRc(digits, cli))
   {
   case CliParm::None:
      if(use == Mandatory) return StreamMissingMandatoryParm;
      if(info != nullptr) return OptionalParmPresent;
      return Ok;
   case CliParm::Ok:
      if(info == nullptr) return OptionalParmMissing;
      break;
   default:
      if(use == Mandatory) return IllegalValueInStream;
      return Ok;
   }

   //  Add the CLI string to DS (currently empty) and compare
   //  it to the one in the message.
   //
   ds.AddDigits(digits);
   if(ds != *info) return ParmValueMismatch;
   return Ok;
}
}