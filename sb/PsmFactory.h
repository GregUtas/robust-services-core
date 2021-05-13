//==============================================================================
//
//  PsmFactory.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifndef PSMFACTORY_H_INCLUDED
#define PSMFACTORY_H_INCLUDED

#include "MsgFactory.h"
#include "SbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  An application subclasses from this if it uses a single PSM (or stack)
//  to implement each of its run-time instances.
//
class PsmFactory : public MsgFactory
{
   friend class MsgPort;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected because this class is virtual.
   //
   PsmFactory
      (Id fid, ContextType type, ProtocolId prid, NodeBase::c_string name);

   //  Protected because subclasses should be singletons.
   //
   virtual ~PsmFactory();

   //  Overridden to create a PsmContext.
   //
   Context* AllocContext() const override;

   //  Overridden to handle an incoming message.  Must NOT be overridden by
   //  applications.  Protected to restrict usage.
   //
   Rc ReceiveMsg
      (Message& msg, bool atIoLevel, TransTrace* tt, Context*& ctx) override;
private:
   //  Creates a PSM that will receive an initial message from LOWER.  The
   //  default version generates a log and must be overridden by applications.
   //
   virtual ProtocolSM* AllocIcPsm
      (const Message& msg, ProtocolLayer& lower) const;

   //  Informs the factory that PORT was just allocated.  MSG is the incoming
   //  message, if any, that created the port.  May be overridden by a factory
   //  that needs to save port.LocAddr().SbAddr() in a user profile.  Note that
   //  LocAddr()'s IP address and port will still be nil at this point.
   //
   virtual void PortAllocated(const MsgPort& port, const Message* msg) const { }
};
}
#endif
