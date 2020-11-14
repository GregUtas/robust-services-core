//==============================================================================
//
//  PotsHtlService.cpp
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
#include "PotsHtlService.h"
#include "ServiceSM.h"
#include "State.h"
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
#include "Singleton.h"

using namespace CallBase;

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
   ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
   EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;
};

//==============================================================================

PotsHtlInitiator::PotsHtlInitiator() : Initiator(PotsHtlServiceId,
   PotsCallServiceId, BcTrigger::CollectInformationSap,
   PotsCollectInformationSap::PotsHtlPriority)
{
   Debug::ft("PotsHtlInitiator.ctor");
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsHtlInitiator::ProcessEvent
   (const ServiceSM& parentSsm, Event& currEvent, Event*& nextEvent) const
{
   Debug::ft("PotsHtlInitiator.ProcessEvent");

   auto& pssm = static_cast< const PotsBcSsm& >(parentSsm);
   auto prof = pssm.Profile();

   if(prof->HasFeature(HTL))
   {
      nextEvent = new InitiationReqEvent(*currEvent.Owner(), PotsHtlServiceId);
      return EventHandler::Initiate;
   }

   return EventHandler::Pass;
}

//==============================================================================

PotsHtlService::PotsHtlService() : Service(PotsHtlServiceId, false, true)
{
   Debug::ft("PotsHtlService.ctor");

   Singleton< PotsHtlNull >::Instance();
}

//------------------------------------------------------------------------------

PotsHtlService::~PotsHtlService()
{
   Debug::ftnt("PotsHtlService.dtor");
}

//------------------------------------------------------------------------------

ServiceSM* PotsHtlService::AllocModifier() const
{
   Debug::ft("PotsHtlService.AllocModifier");

   return new PotsHtlSsm;
}

//==============================================================================

PotsHtlNull::PotsHtlNull() : State(PotsHtlServiceId, ServiceSM::Null)
{
   Debug::ft("PotsHtlNull.ctor");
}

//==============================================================================

PotsHtlSsm::PotsHtlSsm() : ServiceSM(PotsHtlServiceId)
{
   Debug::ft("PotsHtlSsm.ctor");
}

//------------------------------------------------------------------------------

PotsHtlSsm::~PotsHtlSsm()
{
   Debug::ftnt("PotsHtlSsm.dtor");
}

//------------------------------------------------------------------------------

ServicePortId PotsHtlSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft("PotsHtlSsm.CalcPort");

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsHtlSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsHtlSsm.ProcessInitAck");

   auto& pssm = static_cast< PotsBcSsm& >(*Parent());
   auto stid = pssm.CurrState();

   if(stid == BcState::CollectingInformation)
   {
      auto prof = pssm.Profile();
      auto htlp = static_cast< PotsHtlFeatureProfile* >(prof->FindFeature(HTL));
      if(htlp == nullptr) Context::Kill("HTL not assigned", 0);

      auto dn = htlp->GetDN();
      DigitString ds(dn);
      auto dsrc = pssm.DialedDigits().AddDigits(ds);

      if((dsrc == DigitString::IllegalDigit) ||
         (dsrc == DigitString::Overflow))
         pssm.RaiseCollectionTimeout(nextEvent);
      else
         pssm.RaiseLocalInformation(nextEvent);
      return EventHandler::Revert;
   }

   Context::Kill("invalid state", stid);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

EventHandler::Rc PotsHtlSsm::ProcessInitNack
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft("PotsHtlSsm.ProcessInitNack");

   return EventHandler::Resume;
}
}
