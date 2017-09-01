//==============================================================================
//
//  PsmFactory.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef PSMFACTORY_H_INCLUDED
#define PSMFACTORY_H_INCLUDED

#include "MsgFactory.h"
#include "SbTypes.h"

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
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected because this class is virtual.
   //
   PsmFactory(Id fid, ContextType type, ProtocolId prid, const char* name);

   //  Protected because subclasses should be singletons.
   //
   virtual ~PsmFactory();

   //  Overridden to create a PsmContext.
   //
   virtual Context* AllocContext() const override;

   //  Overridden to handle an incoming message.  Must NOT be overridden by
   //  applications.  Protected to restrict usage.
   //
   virtual Rc ReceiveMsg
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
   virtual void PortAllocated
      (const MsgPort& port, const Message* msg) const { }
};
}
#endif
