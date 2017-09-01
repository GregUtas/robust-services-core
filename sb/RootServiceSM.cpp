//==============================================================================
//
//  RootServiceSM.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "RootServiceSM.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "EventHandler.h"
#include "SbEvents.h"
#include "SsmContext.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name RootServiceSM_ctor = "RootServiceSM.ctor";

RootServiceSM::RootServiceSM(ServiceId sid) :
   ServiceSM(sid),
   ctx_(nullptr)
{
   Debug::ft(RootServiceSM_ctor);

   //  Register the SSM with its context.
   //
   ctx_ = static_cast< SsmContext* >(Context::RunningContext());

   if(ctx_ != nullptr) ctx_->SetRoot(this);
}

//------------------------------------------------------------------------------

fn_name RootServiceSM_dtor = "RootServiceSM.dtor";

RootServiceSM::~RootServiceSM()
{
   Debug::ft(RootServiceSM_dtor);

   //  Deregister the SSM from its context.
   //
   if(ctx_ != nullptr) ctx_->SetRoot(nullptr);
}

//------------------------------------------------------------------------------

void RootServiceSM::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   ServiceSM::Display(stream, prefix, options);

   stream << prefix << "ctx : " << ctx_ << CRLF;
}

//------------------------------------------------------------------------------

void RootServiceSM::Patch(sel_t selector, void* arguments)
{
   ServiceSM::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name RootServiceSM_ProcessInitAck = "RootServiceSM.ProcessInitAck";

EventHandler::Rc RootServiceSM::ProcessInitAck
   (Event& icEvent, Event*& nextEvent)
{
   Debug::ft(RootServiceSM_ProcessInitAck);

   Context::Kill(RootServiceSM_ProcessInitAck, Sid(), 0);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name RootServiceSM_ProcessInitNack = "RootServiceSM.ProcessInitNack";

EventHandler::Rc RootServiceSM::ProcessInitNack
   (Event& icEvent, Event*& nextEvent)
{
   Debug::ft(RootServiceSM_ProcessInitNack);

   Context::Kill(RootServiceSM_ProcessInitNack, Sid(), 0);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name RootServiceSM_ProcessSap = "RootServiceSM.ProcessSap";

EventHandler::Rc RootServiceSM::ProcessSap(Event& icEvent, Event*& nextEvent)
{
   Debug::ft(RootServiceSM_ProcessSap);

   Context::Kill(RootServiceSM_ProcessSap, Sid(), 0);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name RootServiceSM_ProcessSip = "RootServiceSM.ProcessSip";

EventHandler::Rc RootServiceSM::ProcessSip(Event& icEvent, Event*& nextEvent)
{
   Debug::ft(RootServiceSM_ProcessSip);

   Context::Kill(RootServiceSM_ProcessSip, Sid(), 0);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name RootServiceSM_ProcessSnp = "RootServiceSM.ProcessSnp";

EventHandler::Rc RootServiceSM::ProcessSnp(Event& icEvent, Event*& nextEvent)
{
   Debug::ft(RootServiceSM_ProcessSnp);

   Context::Kill(RootServiceSM_ProcessSnp, Sid(), 0);
   return EventHandler::Suspend;
}

//------------------------------------------------------------------------------

fn_name RootServiceSM_RaiseProtocolError = "RootServiceSM.RaiseProtocolError";

Event* RootServiceSM::RaiseProtocolError(ProtocolSM& psm, ProtocolSM::Error err)
{
   Debug::ft(RootServiceSM_RaiseProtocolError);

   return new AnalyzeMsgEvent(*Context::ContextMsg());
}
}
