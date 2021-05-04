//==============================================================================
//
//  PotsMultiplexer.h
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
#ifndef POTSMULTIPLEXER_H_INCLUDED
#define POTSMULTIPLEXER_H_INCLUDED

#include "Event.h"
#include "MediaPsm.h"
#include "MediaSsm.h"
#include "Service.h"
#include "SsmFactory.h"
#include "State.h"
#include <cstddef>
#include "BcCause.h"
#include "BcProgress.h"
#include "EventHandler.h"
#include "NbTypes.h"
#include "PotsProtocol.h"
#include "SbTypes.h"
#include "Switch.h"

namespace PotsBase
{
   class PotsProfile;
}

using namespace MediaBase;
using namespace CallBase;
using namespace NodeBase;
using namespace SessionBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsMuxFactory : public SsmFactory
{
   friend class Singleton< PotsMuxFactory >;

   PotsMuxFactory();
   ~PotsMuxFactory();
   CliText* CreateText() const override;
   RootServiceSM* AllocRoot(const Message& msg, ProtocolSM& psm) const override;
   ProtocolSM* AllocIcPsm
      (const Message& msg, ProtocolLayer& lower) const override;
   SsmContext* FindContext(const Message& msg) const override;
   Message* AllocIcMsg(SbIpBufferPtr& buff) const override;
   Message* AllocOgMsg(SignalId sid) const override;
   Message* ReallocOgMsg(SbIpBufferPtr& buff) const override;
};

//------------------------------------------------------------------------------

class PotsMuxPsm : public MediaPsm
{
public:
   static const StateId Active = Idle + 1;

   PotsMuxPsm(ProtocolLayer& adj, bool upper, Switch::PortId port);
   explicit PotsMuxPsm(Switch::PortId port);
   ~PotsMuxPsm();
   Switch::PortId TsPort() const { return header_.port; }
   void SetRemService(ServiceId sid) { remSid_ = sid; }
   void SendSignal(PotsSignal::Id signal);
   void SendFacility(ServiceId sid, Facility::Ind ind);
   void SendFacility(Facility::Ind ind);
   void SendCause(Cause::Ind cause);
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   Message::Route Route() const override;
   IncomingRc ProcessIcMsg(Message& msg, Event*& event) override;
   OutgoingRc ProcessOgMsg(Message& msg) override;
   void SendFinalMsg() override;
   void EnsureMediaMsg() override;

   ServiceId remSid_;
   const Pots_UN_Message* ogMsg_;
   PotsHeaderInfo header_;
   PotsFacilityInfo facility_;
   ProgressInfo progress_;
   CauseInfo cause_;
   bool sendCause_;
};

//------------------------------------------------------------------------------

class PotsMuxService : public Service
{
   friend class Singleton< PotsMuxService >;

   PotsMuxService();
   ~PotsMuxService();
};

//------------------------------------------------------------------------------

class PotsMuxState : public State
{
public:
   static const Id FMUXS = ServiceSM::Null;

   static const Id Null    = FMUXS + 0;
   static const Id Passive = FMUXS + 1;
protected:
   explicit PotsMuxState(Id stid);
   virtual ~PotsMuxState();
};

//------------------------------------------------------------------------------

class PotsMuxEvent : public Event
{
public:
   static const Id Initiate = NextId + 0;
   static const Id Relay    = NextId + 1;
   virtual ~PotsMuxEvent();
protected:
   PotsMuxEvent(Id eid, ServiceSM& owner);
};

//------------------------------------------------------------------------------

class PotsMuxRelayEvent : public PotsMuxEvent
{
public:
   explicit PotsMuxRelayEvent(ServiceSM& owner);
   ~PotsMuxRelayEvent();
};

//------------------------------------------------------------------------------

class PotsMuxSsm : public MediaSsm
{
public:
   typedef int CallId;

   static const CallId MaxCallId = 1;

   PotsMuxSsm(const Message& msg, ProtocolSM& psm);
   ~PotsMuxSsm();
   void SetProfile(PotsProfile* prof) { prof_ = prof; }
   PotsProfile* Profile() const { return prof_; }
   void SetUPsm(PotsCallPsm& psm);
   PotsCallPsm* UPsm() const { return uPsm_; }
   void SetNPsm(CallId cid, PotsMuxPsm& psm);
   PotsMuxPsm* NPsm(CallId cid) const { return nPsm_[cid]; }
   size_t CountCalls() const;
   EventHandler::Rc Initiate(Event*& nextEvent);
   EventHandler::Rc RelayMsg();
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   PotsMuxPsm* NPsm() const;
   ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   void PsmDeleted(ProtocolSM& exPsm) override;

   PotsProfile* prof_;
   PotsCallPsm* uPsm_;
   PotsMuxPsm* nPsm_[MaxCallId + 1];
};

//------------------------------------------------------------------------------

class PotsDiscService : public Service
{
   friend class Singleton< PotsDiscService >;

   PotsDiscService();
   ~PotsDiscService();
   ServiceSM* AllocModifier() const override;
};
}
#endif
