//==============================================================================
//
//  SbHandlers.h
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
#ifndef SBHANDLERS_H_INCLUDED
#define SBHANDLERS_H_INCLUDED

#include "EventHandler.h"

namespace NodeBase
{
   template< class T > class Singleton;
}

//------------------------------------------------------------------------------

namespace SessionBase
{
//  This fixed handler for the AnalyzeMsgEvent invokes the service-specific
//  message analyzer that is registered against the ServicePortId on which
//  the incoming message arrived.
//
class SbAnalyzeMessage : public EventHandler
{
   friend class NodeBase::Singleton< SbAnalyzeMessage >;

   SbAnalyzeMessage() = default;
   ~SbAnalyzeMessage() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//------------------------------------------------------------------------------
//
//  The fixed handler for the AnalyzeSapEvent invokes the modifier's ProcessSap
//  function.
//
class SbAnalyzeSap : public EventHandler
{
   friend class NodeBase::Singleton< SbAnalyzeSap >;

   SbAnalyzeSap() = default;
   ~SbAnalyzeSap() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//------------------------------------------------------------------------------
//
//  The fixed handler for the AnalyzeSnpEvent invokes the modifier's ProcessSnp
//  function.
//
class SbAnalyzeSnp : public EventHandler
{
   friend class NodeBase::Singleton< SbAnalyzeSnp >;

   SbAnalyzeSnp() = default;
   ~SbAnalyzeSnp() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//------------------------------------------------------------------------------
//
//  This fixed handler for the ForceTransitionEvent invokes the event handler
//  specified by the ForceTransitionEvent.
//
class SbForceTransition : public EventHandler
{
   friend class NodeBase::Singleton< SbForceTransition >;

   SbForceTransition() = default;
   ~SbForceTransition() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//------------------------------------------------------------------------------
//
//  The fixed handler for the InitiationReq invokes the modifier's ProcessSip,
//  ProcessInitAck, or ProcessInitAck function.
//
class SbInitiationReq : public EventHandler
{
   friend class NodeBase::Singleton< SbInitiationReq >;

   SbInitiationReq() = default;
   ~SbInitiationReq() = default;
   Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};
}
#endif
