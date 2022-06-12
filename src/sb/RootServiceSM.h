//==============================================================================
//
//  RootServiceSM.h
//
//  Copyright (C) 2013-2022  Greg Utas
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
#ifndef ROOTSERVICESM_H_INCLUDED
#define ROOTSERVICESM_H_INCLUDED

#include "ServiceSM.h"
#include "ProtocolSM.h"
#include "SbTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  A RootServiceSM is the first one created in a processing context that will
//  also contain PSMs.  It cannot be a modifier, but it may be modifiable (that
//  is, it can allow its behavior to be modified by SSMs in its SSMQ).
//
class RootServiceSM : public ServiceSM
{
   friend class SsmContext;
public:
   //  Raises an appropriate event when PSM has detected a protocol error.
   //  This is invoked when an implementation of ProtocolSM::ProcessIcMsg
   //  does not wish to raise AnalyzeMsgEvent.  The default version raises
   //  AnalyzeMsgEvent and may be overridden.
   //
   virtual Event* RaiseProtocolError(ProtocolSM& psm, ProtocolSM::Error err);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Returns the SSM's context.
   //
   SsmContext* GetContext() const override { return ctx_; }

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Uses SID to invoke SetService.  Protected because this class is
   //  virtual.  A root SSM is constructed by SsmFactory::AllocRoot.
   //
   explicit RootServiceSM(ServiceId sid);

   //  Invoked by EndOfTransaction if the SSM is in the Null state or if
   //  the PSM queue is empty (which is an error if the SSM is not in the
   //  Null state).  Deletes any PSMs still queued on the SSM and removes
   //  the SSM from its context.  Also invoked by InvokerPool::Recover to
   //  clean up a context after a fatal error.  Protected to restrict
   //  deletion.  Virtual to allow subclassing.
   //
   virtual ~RootServiceSM();
private:
   //  A root service does not receive an initiation ack, so this kills
   //  the context.
   //
   EventHandler::Rc ProcessInitAck
      (Event& currEvent, Event*& nextEvent) override;

   //  A root service does not receive an initiation nack, so this kills
   //  the context.
   //
   EventHandler::Rc ProcessInitNack
      (Event& currEvent, Event*& nextEvent) override;

   //  A root service does not receive an SAP, so this kills the context.
   //
   EventHandler::Rc ProcessSap(Event& currEvent, Event*& nextEvent) override;

   //  A root service does not receive an SIP, so this kills the context.
   //
   EventHandler::Rc ProcessSip(Event& currEvent, Event*& nextEvent) override;

   //  A root service does not receive an SNP, so this kills the context.
   //
   EventHandler::Rc ProcessSnp(Event& currEvent, Event*& nextEvent) override;

   //  The context in which the SSM is the root.
   //
   SsmContext* ctx_;
};
}
#endif
