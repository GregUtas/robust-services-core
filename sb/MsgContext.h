//==============================================================================
//
//  MsgContext.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef MSGCONTEXT_H_INCLUDED
#define MSGCONTEXT_H_INCLUDED

#include "Context.h"
#include "NbTypes.h"
#include "SbTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Supports a stateless context in which a subclass of MsgFactory receives
//  messages through its MsgFactory::ProcessIcMsg function.
//
class MsgContext : public Context
{
   friend class MsgFactory;
public:
   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Protected to restrict creation.
   //
   explicit MsgContext(Faction faction);

   //  Protected to restrict deletion.  Virtual to allow subclassing.
   //
   virtual ~MsgContext();

   //  Returns the type of context.
   //
   virtual ContextType Type() const override { return SingleMsg; }

   //  Overridden to flag the context message as handled.
   //
   virtual void EndOfTransaction() override;
private:
   //  Overridden to handle the arrival of MSG.
   //
   virtual void ProcessIcMsg(Message& msg) override;
};
}
#endif
