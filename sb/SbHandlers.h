//==============================================================================
//
//  SbHandlers.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SBHANDLERS_H_INCLUDED
#define SBHANDLERS_H_INCLUDED

#include "EventHandler.h"

namespace NodeBase
{
   template< typename T > class Singleton;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  This fixed handler for the AnalyzeMsgEvent invokes the service-specific
//  message analyzer that is registered against the ServicePortId on which
//  the incoming message arrived.
//
class SbAnalyzeMessage : public EventHandler
{
   friend class Singleton< SbAnalyzeMessage >;
private:
   SbAnalyzeMessage() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//------------------------------------------------------------------------------
//
//  The fixed handler for the AnalyzeSapEvent invokes the modifier's ProcessSap
//  function.
//
class SbAnalyzeSap : public EventHandler
{
   friend class Singleton< SbAnalyzeSap >;
private:
   SbAnalyzeSap() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//------------------------------------------------------------------------------
//
//  The fixed handler for the AnalyzeSnpEvent invokes the modifier's ProcessSnp
//  function.
//
class SbAnalyzeSnp : public EventHandler
{
   friend class Singleton< SbAnalyzeSnp >;
private:
   SbAnalyzeSnp() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//------------------------------------------------------------------------------
//
//  This fixed handler for the ForceTransitionEvent invokes the event handler
//  specified by the ForceTransitionEvent.
//
class SbForceTransition : public EventHandler
{
   friend class Singleton< SbForceTransition >;
private:
   SbForceTransition() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};

//------------------------------------------------------------------------------
//
//  The fixed handler for the InitiationReq invokes the modifier's ProcessSip,
//  ProcessInitAck, or ProcessInitAck function.
//
class SbInitiationReq : public EventHandler
{
   friend class Singleton< SbInitiationReq >;
private:
   SbInitiationReq() { }
   virtual Rc ProcessEvent
      (ServiceSM& ssm, Event& currEvent, Event*& nextEvent) const override;
};
}
#endif
