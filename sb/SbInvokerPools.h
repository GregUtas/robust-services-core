//==============================================================================
//
//  SbInvokerPools.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SBINVOKERPOOLS_H_INCLUDED
#define SBINVOKERPOOLS_H_INCLUDED

#include "InvokerPool.h"
#include "NbTypes.h"
#include "SysTypes.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
//  The payload pool is for applications that perform work for end users.
//
class PayloadInvokerPool : public InvokerPool
{
   friend class Singleton< PayloadInvokerPool >;
public:
   //  Overridden to reject ingress work when the ingress work queue gets
   //  too long or the number of available SpIpBuffers gets too low.
   //
   virtual bool RejectIngressWork() const override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   PayloadInvokerPool();

   //  Private because this singleton is not subclassed.
   //
   ~PayloadInvokerPool();

   //  The configuration parameter for the maximum length of
   //  this pool's ingress work queue.
   //
   CfgIntParmPtr noIngressQueueLength_;

   //  The configuration parameter for the number of SbIpBuffers
   //  reserved for non-ingress work.
   //
   CfgIntParmPtr noIngressMessageCount_;

   //  The maximum length allowed for the ingress work queue.
   //
   static word NoIngressQueueLength_;

   //  The number of SbIpBuffers reserved for non-ingress work.
   //
   static word NoIngressMessageCount_;
};
}
#endif
