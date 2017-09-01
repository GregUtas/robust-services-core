//==============================================================================
//
//  MsgFactory.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef MSGFACTORY_H_INCLUDED
#define MSGFACTORY_H_INCLUDED

#include "Factory.h"
#include "SbTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Stateless applications that process a stream of independent messages
//  (a connectionless protocol) subclass from this.
//
class MsgFactory : public Factory
{
   friend class MsgContext;
public:
   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected because this is class is virtual.
   //
   MsgFactory(Id fid, ContextType type, ProtocolId prid, const char* name);

   //  Protected because subclasses should be singletons.
   //
   virtual ~MsgFactory();

   //  Overridden to create a MsgContext.
   //
   virtual Context* AllocContext() const override;

   //  Captures the arrival of MSG at CTX.  TT is the record, if any, that
   //  was created to record the work.
   //
   static void CaptureMsg(Context& ctx, const Message& msg, TransTrace* tt);

   //  Overridden to handle a message arriving at a stateless context.
   //  Must NOT be overridden by applications.  Protected to restrict
   //  usage.
   //
   virtual Rc ReceiveMsg
      (Message& msg, bool atIoLevel, TransTrace* tt, Context*& ctx) override;
private:
   //  Handles an incoming message in a stateless context.  The default
   //  version generates a log and must be overridden by a factory that
   //  uses stateless contexts.  Private to restrict usage.
   //
   virtual void ProcessIcMsg(Message& msg) const;
};
}
#endif
