//==============================================================================
//
//  PotsDiscService.cpp
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
#include "PotsMultiplexer.h"
#include "ServiceSM.h"
#include "BcSessions.h"
#include "Context.h"
#include "Debug.h"
#include "SbAppIds.h"
#include "Singleton.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
class PotsDiscNull : public State
{
   friend class Singleton< PotsDiscNull >;
private:
   PotsDiscNull();
};

class PotsDiscSsm : public ServiceSM
{
public:
   PotsDiscSsm();
   ~PotsDiscSsm();
private:
   ServicePortId CalcPort(const AnalyzeMsgEvent& ame) override;
   EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;
};

//==============================================================================

fn_name PotsDiscService_ctor = "PotsDiscService.ctor";

PotsDiscService::PotsDiscService() : Service(PotsDiscServiceId, false, true)
{
   Debug::ft(PotsDiscService_ctor);

   Singleton< PotsDiscNull >::Instance();
}

//------------------------------------------------------------------------------

fn_name PotsDiscService_dtor = "PotsDiscService.dtor";

PotsDiscService::~PotsDiscService()
{
   Debug::ftnt(PotsDiscService_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsDiscService_AllocModifier = "PotsDiscService.AllocModifier";

ServiceSM* PotsDiscService::AllocModifier() const
{
   Debug::ft(PotsDiscService_AllocModifier);

   return new PotsDiscSsm;
}

//==============================================================================

fn_name PotsDiscNull_ctor = "PotsDiscNull.ctor";

PotsDiscNull::PotsDiscNull() : State(PotsDiscServiceId, ServiceSM::Null)
{
   Debug::ft(PotsDiscNull_ctor);
}

//==============================================================================

fn_name PotsDiscSsm_ctor = "PotsDiscSsm.ctor";

PotsDiscSsm::PotsDiscSsm() : ServiceSM(PotsDiscServiceId)
{
   Debug::ft(PotsDiscSsm_ctor);
}

//------------------------------------------------------------------------------

fn_name PotsDiscSsm_dtor = "PotsDiscSsm.dtor";

PotsDiscSsm::~PotsDiscSsm()
{
   Debug::ftnt(PotsDiscSsm_dtor);
}

//------------------------------------------------------------------------------

fn_name PotsDiscSsm_CalcPort = "PotsDiscSsm.CalcPort";

ServicePortId PotsDiscSsm::CalcPort(const AnalyzeMsgEvent& ame)
{
   Debug::ft(PotsDiscSsm_CalcPort);

   return Parent()->CalcPort(ame);
}

//------------------------------------------------------------------------------

fn_name PotsDiscSsm_ProcessInitAck = "PotsDiscSsm.ProcessInitAck";

EventHandler::Rc PotsDiscSsm::ProcessInitAck
   (Event& currEvent, Event*& nextEvent)
{
   Debug::ft(PotsDiscSsm_ProcessInitAck);

   auto& pssm = static_cast< BcSsm& >(*Parent());
   auto stid = pssm.CurrState();
   auto pmsg = static_cast< PotsMessage* >(Context::ContextMsg());
   auto pci = pmsg->FindType< CauseInfo >(PotsParameter::Cause);

   if((stid == BcState::Null) && (pci != nullptr))
   {
      pssm.RaiseApplyTreatment(nextEvent, pci->cause);
      return EventHandler::Revert;
   }

   Context::Kill("invalid state", stid);
   return EventHandler::Suspend;
}
}
