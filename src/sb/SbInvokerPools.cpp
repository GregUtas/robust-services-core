//==============================================================================
//
//  SbInvokerPools.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "SbInvokerPools.h"
#include <chrono>
#include <cstddef>
#include <ostream>
#include <ratio>
#include <string>
#include "Alarm.h"
#include "AlarmRegistry.h"
#include "CfgIntParm.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "Log.h"
#include "SbLogs.h"
#include "SbPools.h"
#include "SbTypes.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name PayloadInvokerPool_ctor = "PayloadInvokerPool.ctor";

PayloadInvokerPool::PayloadInvokerPool() :
   InvokerPool(PayloadFaction, "NumOfPayloadInvokers"),
   overloadAlarm_(nullptr),
   noIngressQueueLength_(nullptr),
   noIngressMessageCount_(nullptr)
{
   Debug::ft(PayloadInvokerPool_ctor);

   auto reg = Singleton<CfgParmRegistry>::Instance();

   noIngressQueueLength_.reset
      (static_cast<CfgIntParm*>(reg->FindParm("NoIngressQueueLength")));

   if(noIngressQueueLength_ == nullptr)
   {
      noIngressQueueLength_.reset
         (new CfgIntParm("NoIngressQueueLength", "1200", 600, 1800,
         "maximum length of ingress work queue"));
      reg->BindParm(*noIngressQueueLength_);
   }

   noIngressMessageCount_.reset
      (static_cast<CfgIntParm*>(reg->FindParm("NoIngressMessageCount")));

   if(noIngressMessageCount_ == nullptr)
   {
      noIngressMessageCount_.reset
         (new CfgIntParm("NoIngressMessageCount", "100", 50, 500,
         "messages reserved for non-ingress work"));
      reg->BindParm(*noIngressMessageCount_);
   }

   //  Find the overload alarm, which should already have been created.
   //
   auto areg = Singleton<AlarmRegistry>::Instance();
   overloadAlarm_ = areg->Find(OverloadAlarmName);

   if(overloadAlarm_ == nullptr)
   {
      Debug::SwLog(PayloadInvokerPool_ctor, "alarm not found", 0);
   }
}

//------------------------------------------------------------------------------

PayloadInvokerPool::~PayloadInvokerPool()
{
   Debug::ftnt("PayloadInvokerPool.dtor");
}

//------------------------------------------------------------------------------

void PayloadInvokerPool::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   InvokerPool::Display(stream, prefix, options);

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

void PayloadInvokerPool::RecordDelay
   (MsgPriority prio, const nsecs_t& delay) const
{
   Debug::ft("PayloadInvokerPool.RecordDelay");

   InvokerPool::RecordDelay(prio, delay);

   AlarmStatus status = CriticalAlarm;

   auto nsecs = delay.count();

   if(nsecs < (std::nano::den << 1))
      status = NoAlarm;
   else if(nsecs < (std::nano::den << 2))
      status = MinorAlarm;
   else if(nsecs < (std::nano::den << 3))
      status = MajorAlarm;

   if(overloadAlarm_ != nullptr)
   {
      auto log = overloadAlarm_->Create
         (SessionLogGroup, SessionOverload, status);
      if(log != nullptr) Log::Submit(log);
   }
}

//------------------------------------------------------------------------------

bool PayloadInvokerPool::RejectIngressWork() const
{
   Debug::ft("PayloadInvokerPool.RejectIngressWork");

   auto msgAvail = Singleton<MessagePool>::Instance()->AvailCount();
   auto msgOvld = (msgAvail <= size_t(noIngressMessageCount_->CurrValue()));
   auto workLength = WorkQCurrLength(INGRESS);
   auto workOvld = (workLength >= size_t(noIngressQueueLength_->CurrValue()));

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
