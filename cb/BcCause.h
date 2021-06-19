//==============================================================================
//
//  BcCause.h
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
#ifndef BCCAUSE_H_INCLUDED
#define BCCAUSE_H_INCLUDED

#include "TlvIntParameter.h"
#include <cstdint>
#include <iosfwd>
#include <string>
#include "SbTypes.h"
#include "SysTypes.h"

using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace CallBase
{
//  Cause values.  These indicate why a call ended.
//
namespace Cause
{
   //  Type for cause values.
   //
   typedef uint8_t Ind;

   constexpr Ind NilInd = 0;                   // default value
   constexpr Ind UnallocatedNumber = 1;        // destination doesn't exist
   constexpr Ind Confirmation = 2;             // action acknowledged
   constexpr Ind AddressTimeout = 3;           // dialed too slowly
   constexpr Ind NormalCallClearing = 4;       // standard disconnect
   constexpr Ind UserBusy = 5;                 // destination is busy
   constexpr Ind AlertingTimeout = 6;          // destination did not respond
   constexpr Ind AnswerTimeout = 7;            // destination did not answer
   constexpr Ind ExchangeRoutingError = 8;     // network configuration error
   constexpr Ind DestinationOutOfOrder = 9;    // destination out of service
   constexpr Ind InvalidAddress = 10;          // dialed an invalid address
   constexpr Ind FacilityRejected = 11;        // service unavailable
   constexpr Ind TemporaryFailure = 12;        // temporary network problem
   constexpr Ind OutgoingCallsBarred = 13;     // not allowed to place calls
   constexpr Ind IncomingCallsBarred = 14;     // not allowed to receive calls
   constexpr Ind CallRedirected = 15;          // call redirected elsewhere
   constexpr Ind ExcessiveRedirection = 16;    // redirection chain too long
   constexpr Ind MessageInvalidForState = 17;  // unexpected message received
   constexpr Ind ParameterAbsent = 18;         // parameter not received
   constexpr Ind ProtocolTimeout = 19;         // message not received
   constexpr Ind ResetCircuit = 20;            // put circuit in initial state
   constexpr Ind MaxInd = 20;                  // range constant

   //  Returns a string for displaying ID.
   //
   c_string strInd(Ind ind);
}

//------------------------------------------------------------------------------
//
//  Cause value parameter.
//
struct CauseInfo
{
   //  Constructs the nil instance.
   //
   CauseInfo();

   //  Displays member variables, similar to Base::Display.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;

   //  The cause value.
   //
   Cause::Ind cause;
};

//------------------------------------------------------------------------------
//
//  Virtual base class for supporting a CauseInfo parameter.
//
class CauseParameter : public TlvIntParameter< Cause::Ind >
{
protected:
   //  Protected because this class is virtual.
   //
   CauseParameter(ProtocolId prid, Id pid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~CauseParameter();

   //  Overridden to create a CLI parameter for CauseInfo.
   //
   CliParm* CreateCliParm(Usage use) const override;

   //  Overridden to invoke CauseInfo::Display.
   //
   void DisplayMsg(std::ostream& stream, const std::string& prefix,
      const byte_t* bytes, size_t count) const override;
};
}
#endif
