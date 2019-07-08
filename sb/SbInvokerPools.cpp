//==============================================================================
//
//  SbInvokerPools.cpp
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
#include "SbInvokerPools.h"
#include <cstddef>
#include <ostream>
#include <string>
#include "CfgIntParm.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "Message.h"
#include "SbPools.h"
#include "Singleton.h"

using namespace NodeBase;
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
         "messages reserved for non-ingress work"));
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
