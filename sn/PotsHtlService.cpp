//==============================================================================
//
//  PotsHtlService.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "PotsHtlService.h"
#include "BcAddress.h"
#include "BcSessions.h"
#include "Context.h"
#include "Debug.h"
#include "EventHandler.h"
#include "PotsFeatures.h"
#include "PotsHtlFeature.h"
#include "PotsProfile.h"
#include "PotsSessions.h"
#include "SbAppIds.h"
#include "SbEvents.h"
#include "ServiceSM.h"
#include "Singleton.h"
#include "State.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsHtlNull : public State
{
   friend class Singleton< PotsHtlNull >;
private:
   PotsHtlNull();
};

class PotsHtlSsm : public ServiceSM
{
public:
   PotsHtlSsm();
   ~PotsHtlSsm();
private:
   virtual ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   virtual EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   virtual EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;
};

//==============================================================================

fn_name PotsHtlInitiator_ctor = "PotsHtlInitiator.ctor";

PotsHtlInitiator::PotsHtlInitiator() : Initiator(PotsHtlServiceId,
   PotsCallServiceId, BcTrigger::CollectInformationSap,
   PotsCollectInformationSap::PotsHtlPriority)
{
   Debug::ft(PotsHtlInitiator_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsHtlInitiator_ProcessEvent = "PotsHtlInitiator.ProcessEvent";

EventHandler::Rc PotsHtlInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& icEvent, Event*& ogEvent) const
{
   Debug::ft(PotsHtlInitiator_ProcessEvent);

   auto& pssm = static_cast< const PotsBcSsm& >(parentSsm);
   auto prof = pssm.Profile();

   if(prof->HasFeature(HTL))
   {
      ogEvent = new InitiationReqEvent(*icEvent.Owner(), PotsHtlServiceId);
      return EventHandler::Initiate;
   }

   return EventHandler::Pass;
}

//==============================================================================

fn_name PotsHtlService_ctor = "PotsHtlService.ctor";

PotsHtlService::PotsHtlService() : Service(PotsHtlServiceId, false, true)
{
   Debug::ft(PotsHtlService_ctor);

   Singleton< PotsHtlNull >::Instance();
}

//------------------------------------------------------------------------------

fn_name PotsHtlService_dtor = "PotsHtlService.dtor";

PotsHtlService::~PotsHtlService()
{
   Debug::ft(PotsHtlService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsHtlService_AllocModifier = "PotsHtlService.AllocModifier";

ServiceSM* PotsHtlService::AllocModifier() const
{
   Debug::ft(PotsHtlService_AllocModifier);

   return new PotsHtlSsm;
}

//==============================================================================

fn_name PotsHtlNull_ctor = "PotsHtlNull.ctor";

PotsHtlNull::PotsHtlNull() : State(PotsHtlServiceId, ServiceSM::Null)
{
   Debug::ft(PotsHtlNull_ctor);
}

//==============================================================================

fn_name PotsHtlSsm_ctor = "PotsHtlSsm.ctor";

PotsHtlSsm::PotsHtlSsm() : ServiceSM(PotsHtlServiceId)
{
   Debug::ft(PotsHtlSsm_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsHtlSsm_dtor = "PotsHtlSsm.dtor";

PotsHtlSsm::~PotsHtlSsm()
{
   Debug::ft(PotsHtlSsm_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsHtlSsm_CalcPort = "PotsHtlSsm.CalcPort";

ServicePortId PotsHtlSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft(PotsHtlSsm_CalcPort);

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

fn_name PotsHtlSsm_ProcessInitAck = "PotsHtlSsm.ProcessInitAck";

EventHandler::Rc PotsHtlSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsHtlSsm_ProcessInitAck);

   auto& pssm = static_cast< PotsBcSsm& >(*Parent());
   auto stid = pssm.CurrState();

   if(stid == BcState::CollectingInformation)
   {
      auto prof = pssm.Profile();
      auto htlp = static_cast< PotsHtlFeatureProfile* >(prof->FindFeature(HTL));
      if(htlp == nullptr) Context::Kill(PotsHtlSsm_ProcessInitAck, stid, 1);

      auto dn = htlp->GetDN();
      auto ds = DigitString(dn);
      auto dsrc = pssm.DialedDigits().AddDigits(ds);

      if((dsrc == DigitString::IllegalDigit) ||
         (dsrc == DigitString::Overflow))
         pssm.RaiseCollectionTimeout(nextEvent);
      else
         pssm.RaiseLocalInformation(nextEvent);
      return EventHandler::Revert;
   }

   Context::Kill(PotsHtlSsm_ProcessInitAck, stid, 0);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name PotsHtlSsm_ProcessInitNack = "PotsHtlSsm.ProcessInitNack";

EventHandler::Rc PotsHtlSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsHtlSsm_ProcessInitNack);

   return EventHandler::Resume;
}
}