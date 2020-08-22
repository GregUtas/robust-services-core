//==============================================================================
//
//  SsmFactory.h
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
#ifndef SSMFACTORY_H_INCLUDED
#define SSMFACTORY_H_INCLUDED

#include "PsmFactory.h"
#include "SbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  An application subclasses from this if it uses a RootServiceSM to implement
//  each of its run-time instances.
//
class SsmFactory : public PsmFactory
{
public:
   //  Creates a root SSM to receive an initial message from PSM.  The default
   //  version generates a log and must be overridden by applications.
   //
   virtual RootServiceSM* AllocRoot(const Message& msg, ProtocolSM& psm) const;

   //  Creates a PSM that will send an initial message that was just allocated
   //  by AllocOgMsg.  The default version returns nullptr and must be
   //  overridden by factories that use PSMs and that support InjectCommand.
   //
   virtual ProtocolSM* AllocOgPsm(const Message& msg) const;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected because this class is virtual.
   //
   SsmFactory(Id fid, ProtocolId prid, NodeBase::c_string name);

   //  Protected because subclasses should be singletons.
   //
   virtual ~SsmFactory();

   //  Overridden to create an SsmContext.
   //
   Context* AllocContext() const override;
private:
   //  Overridden to handle an incoming message.  Must NOT be overridden by
   //  applications.  Protected to restrict usage.
   //
   Rc ReceiveMsg
      (Message& msg, bool atIoLevel, TransTrace* tt, Context*& ctx) override;

   //  Invoked to find the context to which MSG should be delivered when
   //  MsgHeader.join is set.  The default version generates a log and
   //  must be overridden by applications that use the join operation.
   //  If there is no context to join and MSG should create a new context
   //  instead, clear the join flag (while leaving the initial flag set)
   //  and return nullptr.
   //
   virtual SsmContext* FindContext(const Message& msg) const;
};
}
#endif
