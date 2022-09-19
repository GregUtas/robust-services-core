//==============================================================================
//
//  BotThread.cpp
//
//  Copyright (C) 2019-2022  Greg Utas
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
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
#include "BotThread.h"
#include <chrono>
#include <ostream>
#include <ratio>
#include <string>
#include "BaseBot.h"
#include "BotTrace.h"
#include "Debug.h"
#include "DipProtocol.h"
#include "Duration.h"
#include "Formatters.h"
#include "Singleton.h"
#include "SysTypes.h"
#include "TraceBuffer.h"

using std::ostream;
using std::string;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
BotWakeup::BotWakeup(BotEvent event, uint32_t secs) :
   event_(event),
   expiry_(SteadyTime::Now() + msecs_t(secs * SECS_TO_MS))
{
   Debug::ft("BotWakeup.ctor");
}

//------------------------------------------------------------------------------

uint64_t BotWakeup::TimeToExpiry() const
{
   auto now = SteadyTime::Now();
   if(now > expiry_) return 0;
   auto togo = expiry_ - now;
   return (togo.count() / NS_TO_MS);
}

//------------------------------------------------------------------------------

static bool WakeupIsSorted(const BotWakeup& item1, const BotWakeup& item2)
{
   if(item1.expiry_ < item2.expiry_) return true;
   if(item1.expiry_ > item2.expiry_) return false;
   if(item1.event_ < item2.event_) return true;
   if(item1.event_ > item2.event_) return false;
   return(&item1 < &item2);
}

//==============================================================================

BotThread::BotThread() : Thread(PayloadFaction),
   bot_(nullptr),
   exit_(false)
{
   Debug::ft("BotThread.ctor");

   SetInitialized();
}

//------------------------------------------------------------------------------

c_string BotThread::AbbrName() const
{
   return "dipbot";
}

//------------------------------------------------------------------------------

void BotThread::CancelEvent(BotEvent event)
{
   Debug::ft("BotThread.CancelEvent");

   for(auto w = wakeups_.begin(); w != wakeups_.end(); ++w)
   {
      if(w->event_ == event)
      {
         wakeups_.erase(w);
         return;
      }
   }
}

//------------------------------------------------------------------------------

void BotThread::Destroy()
{
   Debug::ft("BotThread.Destroy");

   Singleton<BotThread>::Destroy();
}

//------------------------------------------------------------------------------

void BotThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "bot     : " << bot_ << CRLF;
   stream << prefix << "exit    : " << exit_ << CRLF;
   stream << prefix << "wakeups :";

   if(wakeups_.empty()) stream << " none";
   stream << CRLF;

   auto lead = prefix + spaces(2);

   for(auto item = wakeups_.cbegin(); item != wakeups_.cend(); ++item)
   {
      stream << lead << "event : " << int(item->event_);
      stream << "  expiry (ms): " << item->TimeToExpiry() << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name BotThread_Enter = "BotThread.Enter";

void BotThread::Enter()
{
   Debug::ft(BotThread_Enter);

   Pause(msecs_t(4 * ONE_SEC));

   bot_ = BaseBot::instance();
   auto rc = bot_->initialise();

   if(rc != 0)
   {
      Debug::SwLog(BotThread_Enter, "failed to initialise bot", rc);
      return;
   }

   msecs_t delay;

   while(true)
   {
      //  If there are any wakeup requests, sleep until the next one is
      //  to be processed, else sleep forever (that is, until the next
      //  message arrives).
      //
      if(wakeups_.empty())
         delay = TIMEOUT_NEVER;
      else
         delay = msecs_t(wakeups_.front().TimeToExpiry());

      auto msg = DeqMsg(delay);

      if(msg != nullptr)
      {
         ProcessMsg(msg);
      }

      ProcessEvents();

      if(exit_) return;
   }
}

//------------------------------------------------------------------------------

void BotThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void BotThread::ProcessEvents()
{
   Debug::ft("BotThread.ProcessEvents");

   //  Run through the events in wakeups_ and process those that have expired
   //  by injecting a BM_Message that contains the event.  They are sorted by
   //  the time when they will expire, so return when one that hasn't expired
   //  is reached.
   //
   while(!wakeups_.empty() && (wakeups_.front().TimeToExpiry() == 0))
   {
      auto event = wakeups_.front().event_;

      wakeups_.pop_front();

      DipIpBufferPtr buff(new DipIpBuffer(MsgIncoming, DipHeaderSize));
      auto msg = reinterpret_cast<BM_Message*>(buff->PayloadPtr());
      msg->header.signal = BM_MESSAGE;
      msg->header.spare = event;
      msg->header.length = 0;
      ProcessMsg(buff.release());
   }
}

//------------------------------------------------------------------------------

void BotThread::ProcessMsg(MsgBuffer* msg) const
{
   Debug::ft("BotThread.ProcessMsg");

   //  A message has arrived.  Have the bot process it and then delete it
   //  (which occurs automatically, because we assign it to a unique_ptr).
   //
   DipIpBufferPtr ipb(static_cast<DipIpBuffer*>(msg));

   if(Debug::TraceOn())
   {
      auto tbuff = Singleton<TraceBuffer>::Instance();

      if(tbuff->ToolIsOn(DipTracer))
      {
         auto rec = new BotTrace(BotTrace::IcMsg, *ipb);
         tbuff->Insert(rec);
      }
   }

   auto message = reinterpret_cast<const DipMessage*>(ipb->HeaderPtr());
   bot_->process_message(*message);
}

//------------------------------------------------------------------------------

void BotThread::QueueEvent(BotEvent event, uint32_t secs)
{
   Debug::ft("BotThread.QueueEvent");

   BotWakeup item(event, secs);
   wakeups_.push_back(item);
   wakeups_.sort(WakeupIsSorted);
}

//------------------------------------------------------------------------------

void BotThread::QueueMsg(DipIpBufferPtr& buff)
{
   Debug::ft("BotThread.QueueMsg");

   EnqMsg(*buff.release());
}
}
