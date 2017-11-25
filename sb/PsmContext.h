//==============================================================================
//
//  PsmContext.h
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
#ifndef PSMCONTEXT_H_INCLUDED
#define PSMCONTEXT_H_INCLUDED

#include "MsgContext.h"
#include "NbTypes.h"
#include "Q1Way.h"
#include "SbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Supports a stateful context in which a subclass of PsmFactory creates
//  a standalone PSM that receives messages via ProtocolSM::ProcessIcMsg.
//
class PsmContext : public MsgContext
{
   friend class PsmFactory;
public:
   //  Returns the first PSM in the PSM queue.
   //
   virtual ProtocolSM* FirstPsm() const override { return psmq_.First(); }

   //  Returns the next PSM in the PSM queue.
   //
   virtual void NextPsm(ProtocolSM*& psm) const override { psmq_.Next(psm); }

   //  Overridden to enumerate all objects that the context owns.
   //
   virtual void GetSubtended(Base* objects[], size_t& count) const override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected to restrict creation.
   //
   explicit PsmContext(Faction faction);

   //  Protected to restrict deletion.  Virtual to allow subclassing.
   //
   virtual ~PsmContext();

   //  Finds the port that should receive MSG.
   //
   MsgPort* FindPort(const Message& msg) const;

   //  Returns the type of context.
   //
   virtual ContextType Type() const override { return SinglePort; }

   //  Returns the first port in the port queue.
   //
   virtual MsgPort* FirstPort() const override { return portq_.First(); }

   //  Returns the next port in the port queue.
   //
   virtual void NextPort(MsgPort*& port) const override { portq_.Next(port); }

   //  Overridden to invoke EndOfTransaction on all PSMs and then delete
   //  those in the idle state.
   //
   virtual void EndOfTransaction() override;

   //  Overridden to determine if the context should be deleted.
   //
   virtual bool IsIdle() const override { return psmq_.Empty(); }
private:
   //  Adds PSM to psmq_, after any PSMs of higher or equal priority.
   //
   virtual void EnqPsm(ProtocolSM& psm) override;

   //  Adds PSM to psmq_, after any PSMs of higher priority.
   //
   virtual void HenqPsm(ProtocolSM& psm) override;

   //  Removes PSM from psmq_.
   //
   virtual void ExqPsm(ProtocolSM& psm) override;

   //  Adds PORT to portq_.
   //
   virtual void EnqPort(MsgPort& psm) override;

   //  Removes PORT from portq_.
   //
   virtual void ExqPort(MsgPort& psm) override;

   //  Overridden to handle the arrival of MSG.
   //
   virtual void ProcessIcMsg(Message& msg) override;

   //  The ports that are running in this context.
   //
   Q1Way< MsgPort > portq_;

   //  The PSMs that are running in this context.
   //
   Q1Way< ProtocolSM > psmq_;
};
}
#endif
