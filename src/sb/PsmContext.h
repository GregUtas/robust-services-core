//==============================================================================
//
//  PsmContext.h
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef PSMCONTEXT_H_INCLUDED
#define PSMCONTEXT_H_INCLUDED

#include "MsgContext.h"
#include "MsgPort.h"
#include "NbTypes.h"
#include "ProtocolSM.h"
#include "Q1Way.h"
#include "SbTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Supports a stateful context in which a subclass of PsmFactory creates
//  a stand-alone PSM that receives messages via ProtocolSM::ProcessIcMsg.
//
class PsmContext : public MsgContext
{
   friend class PsmFactory;
public:
   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Returns the first PSM in the PSM queue.
   //
   ProtocolSM* FirstPsm() const override { return psmq_.First(); }

   //  Overridden to enumerate all objects that the context owns.
   //
   void GetSubtended(std::vector<Base*>& objects) const override;

   //  Returns the next PSM in the PSM queue.
   //
   void NextPsm(ProtocolSM*& psm) const override { psmq_.Next(psm); }

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected to restrict creation.
   //
   explicit PsmContext(NodeBase::Faction faction);

   //  Protected to restrict deletion.  Virtual to allow subclassing.
   //
   virtual ~PsmContext();

   //  Finds the port that should receive MSG.
   //
   MsgPort* FindPort(const Message& msg) const;

   //  Returns the first port in the port queue.
   //
   MsgPort* FirstPort() const { return portq_.First(); }

   //  Overridden to invoke EndOfTransaction on all PSMs and then delete
   //  those in the idle state.
   //
   void EndOfTransaction() override;

   //  Overridden to determine if the context should be deleted.
   //
   bool IsIdle() const override { return psmq_.Empty(); }

   //  Returns the next port in the port queue.
   //
   void NextPort(MsgPort*& port) const override { portq_.Next(port); }

   //  Returns the type of context.
   //
   ContextType Type() const override { return SinglePort; }
private:
   //  Adds PORT to portq_.
   //
   void EnqPort(MsgPort& port) override;

   //  Adds PSM to psmq_, after any PSMs of higher or equal priority.
   //
   void EnqPsm(ProtocolSM& psm) override;

   //  Removes PORT from portq_.
   //
   void ExqPort(MsgPort& port) override;

   //  Removes PSM from psmq_.
   //
   void ExqPsm(ProtocolSM& psm) override;

   //  Adds PSM to psmq_, after any PSMs of higher priority.
   //
   void HenqPsm(ProtocolSM& psm) override;

   //  Overridden to handle the arrival of MSG.
   //
   void ProcessIcMsg(Message& msg) override;

   //  The ports that are running in this context.
   //
   NodeBase::Q1Way<MsgPort> portq_;

   //  The PSMs that are running in this context.
   //
   NodeBase::Q1Way<ProtocolSM> psmq_;
};
}
#endif
