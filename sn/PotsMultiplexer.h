//==============================================================================
//
//  PotsMultiplexer.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef POTSMULTIPLEXER_H_INCLUDED
#define POTSMULTIPLEXER_H_INCLUDED

#include "BcCause.h"
#include <cstddef>
#include "BcProgress.h"
#include "Event.h"
#include "EventHandler.h"
#include "MediaPsm.h"
#include "MediaSsm.h"
#include "NbTypes.h"
#include "PotsProtocol.h"
#include "SbTypes.h"
#include "Service.h"
#include "SsmFactory.h"
#include "State.h"
#include "Switch.h"

namespace PotsBase
{
   class PotsProfile;
}

using namespace NodeBase;
using namespace SessionBase;
using namespace MediaBase;
using namespace CallBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsMuxFactory : public SsmFactory
{
   friend class Singleton< PotsMuxFactory >;
private:
   PotsMuxFactory();
   ~PotsMuxFactory();
   virtual CliText* CreateText() const override;
   virtual RootServiceSM* AllocRoot
      (const Message& msg, ProtocolSM& psm) const override;
   virtual ProtocolSM* AllocIcPsm
      (const Message& msg, ProtocolLayer& lower) const override;
   virtual SsmContext* FindContext(const Message& msg) const override;
   virtual Message* AllocIcMsg(SbIpBufferPtr& buff) const override;
   virtual Message* AllocOgMsg(SignalId sid) const override;
   virtual Message* ReallocOgMsg(SbIpBufferPtr& buff) const override;
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
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   virtual Message::Route Route() const override;
   virtual IncomingRc ProcessIcMsg(Message& msg, Event*& event) override;
   virtual OutgoingRc ProcessOgMsg(Message& msg) override;
   virtual void SendFinalMsg() override;
   virtual void EnsureMediaMsg() override;

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
private:
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
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   PotsMuxPsm* NPsm() const;
   virtual ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   virtual void PsmDeleted(ProtocolSM& exPsm) override;

   PotsProfile* prof_;
   PotsCallPsm* uPsm_;
   PotsMuxPsm* nPsm_[MaxCallId + 1];
};

//------------------------------------------------------------------------------

class PotsDiscService : public Service
{
   friend class Singleton< PotsDiscService >;
private:
   PotsDiscService();
   ~PotsDiscService();
   virtual ServiceSM* AllocModifier() const override;
};
}
#endif
