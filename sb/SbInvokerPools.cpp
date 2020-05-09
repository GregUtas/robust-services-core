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
#include "Alarm.h"
#include "AlarmRegistry.h"
#include "CfgIntParm.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Duration.h"
#include "Formatters.h"
#include "Log.h"
#include "SbLogs.h"
#include "SbPools.h"
#include "SbTypes.h"
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
   overloadAlarm_(nullptr),
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

   //  Find the overload alarm, which should already have been created.
   //
   auto areg = Singleton< AlarmRegistry >::Instance();
   overloadAlarm_ = areg->Find(OverloadAlarmName);

   if(overloadAlarm_ == nullptr)
   {
      Debug::SwLog(PayloadInvokerPool_ctor, "alarm not found", 0);
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
   stream << prefix << "overloadAlarm         : ";
   stream << strObj(overloadAlarm_) << CRLF;
}

//------------------------------------------------------------------------------

void PayloadInvokerPool::Patch(sel_t selector, void* arguments)
{
   InvokerPool::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name PayloadInvokerPool_RecordDelay = "PayloadInvokerPool.RecordDelay";

void PayloadInvokerPool::RecordDelay
   (MsgPriority prio, const Duration& delay) const
{
   Debug::ft(PayloadInvokerPool_RecordDelay);

   InvokerPool::RecordDelay(prio, delay);

   AlarmStatus status = CriticalAlarm;

   if(delay < (ONE_SEC << 1))
      status = NoAlarm;
   else if(delay < (ONE_SEC << 2))
      status = MinorAlarm;
   else if(delay < (ONE_SEC << 3))
      status = MajorAlarm;

   if(overloadAlarm_ != nullptr)
   {
      auto log = overloadAlarm_->Create
         (SessionLogGroup, SessionOverload, status);
      if(log != nullptr) Log::Submit(log);
   }
}

//------------------------------------------------------------------------------

fn_name PayloadInvokerPool_RejectIngressWork =
   "PayloadInvokerPool.RejectIngressWork";

bool PayloadInvokerPool::RejectIngressWork() const
{
   Debug::ft(PayloadInvokerPool_RejectIngressWork);

   auto msgCount = Singleton< MessagePool >::Instance()->AvailCount();
   auto msgOvld = (msgCount <= size_t(NoIngressMessageCount_));
   auto workLength = WorkQCurrLength(INGRESS);
   auto workOvld = (workLength >= size_t(NoIngressQueueLength_));

   if(msgOvld || workOvld)
   {
      if(overloadAlarm_ != nullptr)
      {
         auto log = overloadAlarm_->Create
            (SessionLogGroup, SessionOverload, MajorAlarm);
         if(log != nullptr) Log::Submit(log);
      }

      return true;
   }

   if(overloadAlarm_ != nullptr)
   {
      auto log = overloadAlarm_->Create
         (SessionLogGroup, SessionNoOverload, NoAlarm);
      if(log != nullptr) Log::Submit(log);
   }

   return false;
}
}
