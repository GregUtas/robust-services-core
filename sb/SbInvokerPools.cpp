//==============================================================================
//
//  SbInvokerPools.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "SbInvokerPools.h"
#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include "CfgIntParm.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "Message.h"
#include "SbPools.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
word PayloadInvokerPool::NoIngressQueueLength_ = 1200;
word PayloadInvokerPool::NoIngressMessageCount_ = 800;

//------------------------------------------------------------------------------

fn_name PayloadInvokerPool_ctor = "PayloadInvokerPool.ctor";

PayloadInvokerPool::PayloadInvokerPool() :
   InvokerPool(PayloadFaction, "NumOfPayloadInvokers"),
   noIngressQueueLength_(nullptr),
   noIngressMessageCount_(nullptr)
{
   Debug::ft(PayloadInvokerPool_ctor);

   auto reg = Singleton< CfgParmRegistry >::Instance();

   noIngressQueueLength_.reset
      (static_cast< CfgIntParm* >(reg->FindParm("NoIngressQueueLength")));

   if(noIngressQueueLength_ == nullptr)
   {
      noIngressQueueLength_.reset
         (new CfgIntParm("NoIngressQueueLength", "1200",
         &NoIngressQueueLength_, 600, 1800,
         "maximum length of ingress work queue"));
      reg->BindParm(*noIngressQueueLength_);
   }

   noIngressMessageCount_.reset
      (static_cast< CfgIntParm* >(reg->FindParm("NoIngressMessageCount")));

   if(noIngressMessageCount_ == nullptr)
   {
      noIngressMessageCount_.reset
         (new CfgIntParm("NoIngressMessageCount", "800",
         &NoIngressMessageCount_, 400, 1200,
         "Messages reserved for non-ingress work"));
      reg->BindParm(*noIngressMessageCount_);
   }
}

//------------------------------------------------------------------------------

fn_name PayloadInvokerPool_dtor = "PayloadInvokerPool.dtor";

PayloadInvokerPool::~PayloadInvokerPool()
{
   Debug::ft(PayloadInvokerPool_dtor);
}

//------------------------------------------------------------------------------

void PayloadInvokerPool::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   InvokerPool::Display(stream, prefix, options);

   stream << prefix << "NoIngressQueueLength  : ";
   stream << NoIngressQueueLength_ << CRLF;
   stream << prefix << "NoIngressMessageCount : ";
   stream << NoIngressMessageCount_ << CRLF;
   stream << prefix << "noIngressQueueLength  : ";
   stream << strObj(noIngressQueueLength_.get()) << CRLF;
   stream << prefix << "noIngressMessageCount : ";
   stream << strObj(noIngressMessageCount_.get()) << CRLF;
}

//------------------------------------------------------------------------------

void PayloadInvokerPool::Patch(sel_t selector, void* arguments)
{
   InvokerPool::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name PayloadInvokerPool_RejectIngressWork =
   "PayloadInvokerPool.RejectIngressWork";

bool PayloadInvokerPool::RejectIngressWork() const
{
   Debug::ft(PayloadInvokerPool_RejectIngressWork);

   auto msgs = Singleton< MessagePool >::Instance();

   if(msgs->AvailCount() <= size_t(NoIngressMessageCount_)) return true;

   return (WorkQCurrLength(Message::Ingress) >= size_t(NoIngressQueueLength_));
}
}
