//==============================================================================
//
//  SsmContext.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SSMCONTEXT_H_INCLUDED
#define SSMCONTEXT_H_INCLUDED

#include "PsmContext.h"
#include "NbTypes.h"
#include "SbTypes.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Supports stateful contexts in which a subclass of SsmFactory creates a
//  root SSM that receives messages through its PSMs.
//
class SsmContext : public PsmContext
{
   friend class RootServiceSM;
   friend class SsmFactory;
public:
   //  Returns the root SSM.
   //
   virtual RootServiceSM* RootSsm() const override { return root_; }

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
   //  Returns the type of context.
   //
   virtual ContextType Type() const override { return MultiPort; }

   //  Overridden to invoke EndOfTransaction on the root SSM.
   //
   virtual void EndOfTransaction() override;
private:
   //  Private to restrict creation.
   //
   explicit SsmContext(Faction faction);

   //  Private to restrict deletion.  Not subclassed.
   //
   ~SsmContext();

   //  Overridden to handle the arrival of MSG.
   //
   virtual void ProcessIcMsg(Message& msg) override;

   //  Overridden to determine if the context should be deleted.
   //
   virtual bool IsIdle() const override;

   //  Allocates the root SSM that will receive MSG.  PSM is the
   //  uppermost PSM in the stack that MSG just created.
   //
   static RootServiceSM* AllocRoot(const Message& msg, ProtocolSM& psm);

   //  Sets the root SSM.
   //
   void SetRoot(RootServiceSM* root);

   //  Generates a log containing SID, ERRVAL, and the context's
   //  message trace when an error occurs.
   //
   void OutputLog(ServiceId sid, word errval) const;

   //  The root SSM.
   //
   RootServiceSM* root_;
};
}
#endif
