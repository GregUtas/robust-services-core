//==============================================================================
//
//  BcCause.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef BCCAUSE_H_INCLUDED
#define BCCAUSE_H_INCLUDED

#include "TlvIntParameter.h"
#include <cstdint>
#include <iosfwd>
#include <string>
#include "SbTypes.h"

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

   constexpr Ind NilInd                 = 0;  // default value
   constexpr Ind UnallocatedNumber      = 1;  // destination doesn't exist
   constexpr Ind Confirmation           = 2;  // action acknowledged
   constexpr Ind AddressTimeout         = 3;  // dialed too slowly
   constexpr Ind NormalCallClearing     = 4;  // standard disconnect
   constexpr Ind UserBusy               = 5;  // destination is busy
   constexpr Ind AlertingTimeout        = 6;  // destination did not respond
   constexpr Ind AnswerTimeout          = 7;  // destination did not answer
   constexpr Ind ExchangeRoutingError   = 8;  // network configuration error
   constexpr Ind DestinationOutOfOrder  = 9;  // destination out of service
   constexpr Ind InvalidAddress         = 10; // dialed an invalid address
   constexpr Ind FacilityRejected       = 11; // service unavailable
   constexpr Ind TemporaryFailure       = 12; // temporary network problem
   constexpr Ind OutgoingCallsBarred    = 13; // not allowed to place calls
   constexpr Ind IncomingCallsBarred    = 14; // not allowed to receive calls
   constexpr Ind CallRedirected         = 15; // call redirected elsewhere
   constexpr Ind ExcessiveRedirection   = 16; // redirection chain too long
   constexpr Ind MessageInvalidForState = 17; // unexpected message received
   constexpr Ind ParameterAbsent        = 18; // parameter not received
   constexpr Ind ProtocolTimeout        = 19; // message not received
   constexpr Ind ResetCircuit           = 20; // put circuit in initial state
   constexpr Ind MaxInd                 = 20; // range constant

   //  Returns a string for displaying ID.
   //
   const char* strInd(Ind ind);
}

//------------------------------------------------------------------------------
//
//  Cause value parameter.
//
struct CauseInfo
{
public:
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

   //  Overridden to invoke CauseInfo::Display.
   //
   virtual void DisplayMsg(std::ostream& stream, const std::string& prefix,
      const byte_t* bytes, size_t count) const override;

   //  Overridden to create a CLI parameter for CauseInfo.
   //
   virtual CliParm* CreateCliParm(Usage use) const override;
};
}
#endif
