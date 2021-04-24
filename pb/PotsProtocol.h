//==============================================================================
//
//  PotsProtocol.h
//
//  Copyright (C) 2013-2020  Greg Utas
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
#ifndef POTSPROTOCOL_H_INCLUDED
#define POTSPROTOCOL_H_INCLUDED

#include "MediaPsm.h"
#include "Signal.h"
#include "TlvMessage.h"
#include "TlvParameter.h"
#include "TlvProtocol.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "BcCause.h"
#include "Duration.h"
#include "NbTypes.h"
#include "SbTypes.h"
#include "Switch.h"

namespace CallBase
{
   class DigitString;
   struct ProgressInfo;
}

using namespace NodeBase;
using namespace SessionBase;
using namespace MediaBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
//  POTS protocol.
//
class PotsProtocol : public TlvProtocol
{
   friend class Singleton< PotsProtocol >;
public:
   //  Timer values.
   //
   static const secs_t FirstDigitTimeout = 10;  // dial tone to first digit
   static const secs_t InterDigitTimeout = 10;  // between subsequent digits
   static const secs_t RingingCycleTime = 6;    // length of one ringing cycle
   static const secs_t AlertingTimeout = 6;     // ringing to alerting message
   static const secs_t AnswerTimeout = 60;      // call presentation to offhook
   static const secs_t SuspendTimeout = 10;     // TBC onhook to call takedown

   //  Timer identifiers.
   //
   static const TimerId CollectionTimeoutId = 1;  // for digit timeouts
   static const TimerId AlertingTimeoutId = 2;    // for alerting timeout
   static const TimerId AnswerTimeoutId = 3;      // for answer timeout
   static const TimerId SuspendTimeoutId = 4;     // for suspend timeout
   static const TimerId TreatmentTimeoutId = 5;   // for a timed treatment
private:
   //  Private because this is a singleton.
   //
   PotsProtocol();

   //  Private because this is a singleton.
   //
   ~PotsProtocol();
};

//------------------------------------------------------------------------------
//
//  Base class for POTS signals.
//
class PotsSignal : public Signal
{
public:
   //  Identifiers for POTS signals.
   //
   static const Id Offhook   = NextId;
   static const Id Digits    = NextId + 1;
   static const Id Alerting  = NextId + 2;
   static const Id Flash     = NextId + 3;
   static const Id Onhook    = NextId + 4;
   static const Id Facility  = NextId + 5;  // for supplementary services
   static const Id Progress  = NextId + 6;  // media update only
   static const Id Supervise = NextId + 7;  // control of active circuit
   static const Id Lockout   = NextId + 8;  // idle circuit; report onhook
   static const Id Release   = NextId + 9;  // idle circuit; report offhook
   static const Id LastId    = NextId + 9;  // range constant
protected:
   //  Protected because this class is virtual.
   //
   explicit PotsSignal(Id sid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~PotsSignal() = default;
};

//------------------------------------------------------------------------------
//
//  Base class for POTS parameters.
//
class PotsParameter : public TlvParameter
{
public:
   //  Identifiers for POTS parameters.
   //
   static const Id Header   = NextId;      // signal and circuit ID
   static const Id Digits   = NextId + 1;  // digit string
   static const Id Ring     = NextId + 2;  // start/stop ringing
   static const Id Scan     = NextId + 3;  // whether to report digits/flash
   static const Id Media    = NextId + 4;  // media update
   static const Id Cause    = NextId + 5;  // cause value for call takedown
   static const Id Progress = NextId + 6;  // progress indicator
   static const Id Facility = NextId + 7;  // service-specific indicator
   static const Id LastId   = NextId + 7;  // range constant
protected:
   //  Protected because this class is virtual.
   //
   explicit PotsParameter(Id pid);

   //  Protected because subclasses should be singletons.
   //
   virtual ~PotsParameter() = default;
};

//------------------------------------------------------------------------------
//
//  Header for all POTS messages.
//
struct PotsHeaderInfo
{
   //  Constructs the nil instance.
   //
   PotsHeaderInfo();

   //  Displays member variables, similar to Base::Display.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;

   //  The message's signal.
   //
   PotsSignal::Id signal;

   //  The port on which the circuit appears.
   //
   Switch::PortId port;
};

//------------------------------------------------------------------------------
//
//  Parameter for controlling ringing.
//
struct PotsRingInfo
{
   //  Constructs the nil instance.
   //
   PotsRingInfo();

   //  Displays member variables, similar to Base::Display.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;

   //  Specifies whether ringing should be started or stopped.
   //
   bool on : 1;
};

//------------------------------------------------------------------------------
//
//  Parameter for controlling scanning.
//
struct PotsScanInfo
{
   //  Constructs the nil instance.
   //
   PotsScanInfo();

   //  Displays member variables, similar to Base::Display.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;

   //  Set if digits are to be reported.
   //
   bool digits : 1;

   //  Set if a flash is to be reported.
   //
   bool flash : 1;
};

//------------------------------------------------------------------------------
//
//  Service indicators.
//
class Facility
{
public:
   //  Deleted because this class only has static members.
   //
   Facility() = delete;

   //  The type for a service indicator.
   //
   typedef uint8_t Ind;

   static const Ind NilInd = 0;          // default value
   static const Ind InitiationReq = 1;   // service initiation request
   static const Ind InitiationAck = 2;   // service initiation succeeded
   static const Ind InitiationNack = 3;  // service initiation failed
   static const Ind NextInd = 4;         // next available indicator
};

//------------------------------------------------------------------------------
//
//  Parameter for service indicators.
//
struct PotsFacilityInfo
{
   //  Constructs the nil instance.
   //
   PotsFacilityInfo();

   //  Displays member variables, similar to Base::Display.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;

   //  The identifier of the service for which the parameter is intended.
   //
   ServiceId sid;

   //  The service indicator.
   //
   Facility::Ind ind;
};

//------------------------------------------------------------------------------
//
//  Base class for POTS messages.
//
class PotsMessage : public TlvMessage
{
public:
   //  Constructs a message to receive BUFF.
   //
   explicit PotsMessage(SbIpBufferPtr& buff);

   //  Constructs a message, initially of SIZE bytes, to be sent by PSM.
   //
   PotsMessage(ProtocolSM* psm, size_t size);

   //  Virtual to allow subclassing.
   //
   virtual ~PotsMessage();

   //  Adds HEADER to the message.
   //
   PotsHeaderInfo* AddHeader(const PotsHeaderInfo& header);

   //  Adds FACILITY to the message.
   //
   PotsFacilityInfo* AddFacility(const PotsFacilityInfo& facility);

   //  Adds PROGRESS to the message.
   //
   ProgressInfo* AddProgress(const ProgressInfo& progress);

   //  Adds MEDIA to the message.
   //
   MediaInfo* AddMedia(const MediaInfo& media);

   //  Adds CAUSE to the message.
   //
   CauseInfo* AddCause(const CauseInfo& cause);
};

//------------------------------------------------------------------------------
//
//  POTS user-to-network message.
//
class Pots_UN_Message : public PotsMessage
{
public:
   //  Constructs a message to receive BUFF.
   //
   explicit Pots_UN_Message(SbIpBufferPtr& buff);

   //  Constructs a message, initially of SIZE bytes, to be sent by PSM.
   //
   Pots_UN_Message(ProtocolSM* psm, size_t size);

   //  Not subclassed.
   //
   ~Pots_UN_Message();

   //  Adds DIGITS to the message.
   //
   DigitString* AddDigits(const DigitString& digits);
};

//------------------------------------------------------------------------------
//
//  POTS network-to-user message.
//
class Pots_NU_Message : public PotsMessage
{
public:
   //  Constructs a message to receive BUFF.
   //
   explicit Pots_NU_Message(SbIpBufferPtr& buff);

   //  Constructs a message, initially of SIZE bytes, to be sent by PSM.
   //
   Pots_NU_Message(ProtocolSM* psm, size_t size);

   //  Not subclassed.
   //
   ~Pots_NU_Message();

   //  Adds RING to the message.
   //
   PotsRingInfo* AddRing(const PotsRingInfo& ring);

   //  Adds SCAN to the message.
   //
   PotsScanInfo* AddScan(const PotsScanInfo& scan);
};

//------------------------------------------------------------------------------
//
//  POTS user-side PSM.
//
class PotsCallPsm : public MediaPsm
{
public:
   //  There are only two states: idle and active.
   //
   static const StateId Active = Idle + 1;

   //  Creates a PSM that will send an initial message.  PORT is timeswitch
   //  port assigned to the POTS circuit associated with the PSM.
   //
   explicit PotsCallPsm(Switch::PortId port);

   //  Creates a PSM from an adjacent layer.  PORT is the same as above.
   //  The other arguments are the same as those for the base class.
   //
   PotsCallPsm(ProtocolLayer& adj, bool upper, Switch::PortId port);

   //  Returns PSM, cast to a PotsCallPsm, if its factory is PotsCallFactoryId.
   //
   static PotsCallPsm* Cast(ProtocolSM* psm);

   //  Returns the timeswitch port assigned to the PSM.
   //
   Switch::PortId TsPort() const { return header_.port; }

   //  Prepares to send SIGNAL at the end of the transaction.
   //
   void SendSignal(SignalId signal);

   //  Invoked to start/stop reporting digits.
   //
   void ReportDigits(bool report);

   //  Invoked to start/stop reporting flashes.
   //
   void ReportFlash(bool report);

   //  Invoked to start/stop ringing.
   //
   void ApplyRinging(bool on);

   //  Invoked to send CAUSE.
   //
   void SendCause(Cause::Ind cause);

   //  Invoked to send a facility parameter with indicator IND to
   //  the service identified by SID.
   //
   void SendFacility(ServiceId sid, Facility::Ind ind);

   //  Returns the outgoing message (if any) that has been created
   //  to prepare to send a message at the end of the transaction.
   //
   Pots_NU_Message* AccessOgMsg() const { return ogMsg_; }

   //  Synchronizes the PSM with UPSM.  Used when a multiplexer creates a
   //  user-side PSM to take over communication with the POTS circuit.
   //
   void Synch(PotsCallPsm& upsm) const;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private to restrict deletion.  Not subclassed.
   //
   ~PotsCallPsm();

   //  Overridden to allocate a message when a media update is required.
   //
   void EnsureMediaMsg() override;

   //  Overridden to return the route for outgoing messages.
   //
   Message::Route Route() const override;

   //  Overridden to handle an incoming message.
   //
   IncomingRc ProcessIcMsg(Message& msg, Event*& event) override;

   //  Overridden to handle an outgoing message.
   //
   OutgoingRc ProcessOgMsg(Message& msg) override;

   //  Overridden to send a final message if the PSM's context dies.
   //
   void SendFinalMsg() override;

   //  Overridden to inject a final message if the PSM's peer dies.
   //
   void InjectFinalMsg() override;

   //  An empty message that will be finalized and sent at the end
   //  of the transaction.
   //
   Pots_NU_Message* ogMsg_;

   //  Set if ring_ was modified during the transaction.
   //
   bool sendRing_;

   //  Set if scan_ was modified during the transaction.
   //
   bool sendScan_;

   //  Set if cause_ was modified during the transaction.
   //
   bool sendCause_;

   //  Set if facility_ was modified during the transaction.
   //
   bool sendFacility_;

   //  The header for outgoing messages.  The SIGNAL field is updated
   //  by SendSignal.  The PORT field is set when the PSM is created
   //  and remains fixed.
   //
   PotsHeaderInfo header_;

   //  The most recent value for the ring parameter.
   //
   PotsRingInfo ring_;

   //  The most recent value for the scan parameter.
   //
   PotsScanInfo scan_;

   //  The most recent value for the cause parameter.
   //
   CauseInfo cause_;

   //  The most recent value for the facility parameter.
   //
   PotsFacilityInfo facility_;
};
}
#endif
