//==============================================================================
//
//  BotThread.cpp
//
//  Copyright (C) 2019  Greg Utas
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
#include <ostream>
#include <string>
#include <utility>
#include "BaseBot.h"
#include "BotTrace.h"
#include "Debug.h"
#include "DipProtocol.h"
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
fn_name BotThread_ctor = "BotThread.ctor";

BotThread::BotThread() : Thread(PayloadFaction),
   bot_(nullptr),
   exit_(false)
{
   Debug::ft(BotThread_ctor);
}

//------------------------------------------------------------------------------

const char* BotThread::AbbrName() const
{
   return "dipbot";
}

//------------------------------------------------------------------------------

fn_name BotThread_CancelEvent = "BotThread.CancelEvent";

void BotThread::CancelEvent(BotEvent event)
{
   Debug::ft(BotThread_CancelEvent);

   for(auto w = wakeups_.begin(); w != wakeups_.end(); ++w)
   {
      if(w->event == event)
      {
         wakeups_.erase(w);
         return;
      }
   }
}

//------------------------------------------------------------------------------

fn_name BotThread_Destroy = "BotThread.Destroy";

void BotThread::Destroy()
{
   Debug::ft(BotThread_Destroy);

   Singleton< BotThread >::Destroy();
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
      stream << lead << "event : " << int(item->event);
      stream << "  secs : " << int(item->secs) << CRLF;
   }
}

//------------------------------------------------------------------------------

fn_name BotThread_Enter = "BotThread.Enter";

void BotThread::Enter()
{
   Debug::ft(BotThread_Enter);

   Pause(4 * TIMEOUT_1_SEC);

   bot_ = BaseBot::instance();
   auto rc = bot_->initialise();

   if(rc != 0)
   {
      Debug::SwLog(BotThread_Enter, "Failed to initialize bot", rc);
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
         delay = 1000 * wakeups_.cbegin()->secs;

      auto msg = DeqMsg(delay);

      if(msg != nullptr)
         ProcessMsg(msg);
      else
         ProcessEvent();

      if(exit_) return;
   }
}

//------------------------------------------------------------------------------

void BotThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name BotThread_ProcessEvent = "BotThread.ProcessEvent";

void BotThread::ProcessEvent()
{
   Debug::ft(BotThread_ProcessEvent);

   //  The event to be processed is at the front of wakeups_.  Before
   //  injecting a BM_Message containing the event, adjust the timeouts
   //  of the remaining events.  The elements of a set are sorted, so
   //  they're normally read-only.  But .secs is marked mutable as a
   //  hack so that we don't have to delete and reinsert each item.
   //  Because all are adjusted by the same amount, they stay sorted.
   //
   if(wakeups_.empty()) return;
   auto first = wakeups_.begin();
   auto event = first->event;
   auto elapsed = first->secs;

   for(auto item = wakeups_.begin(); item != wakeups_.end(); ++item)
   {
      item->secs -= elapsed;
   }

   wakeups_.erase(first);

   DipIpBufferPtr buff(new DipIpBuffer(MsgIncoming, DipHeaderSize));
   if(buff == nullptr) return;
   auto msg = reinterpret_cast< BM_Message* >(buff->PayloadPtr());
   msg->header.signal = BM_MESSAGE;
   msg->header.spare = event;
   msg->header.length = 0;
   ProcessMsg(buff.release());
}

//------------------------------------------------------------------------------

fn_name BotThread_ProcessMsg = "BotThread.ProcessMsg";

void BotThread::ProcessMsg(MsgBuffer* msg) const
{
   Debug::ft(BotThread_ProcessMsg);

   //  A message has arrived.  Have the bot process it and then delete it
   //  (which occurs automatically, because we assign it to a unique_ptr).
   //
   DipIpBufferPtr ipb(static_cast< DipIpBuffer* >(msg));

   if(Debug::TraceOn())
   {
      if(Singleton< TraceBuffer >::Instance()->ToolIsOn(DipTracer))
      {
         new BotTrace(BotTrace::IcMsg, *ipb);
      }
   }

   auto message = reinterpret_cast< const DipMessage* >(ipb->HeaderPtr());
   bot_->process_message(*message);
}

//------------------------------------------------------------------------------

fn_name BotThread_QueueEvent = "BotThread.QueueEvent";

bool BotThread::QueueEvent(BotEvent event, secs_t secs)
{
   Debug::ft(BotThread_QueueEvent);

   auto item = wakeups_.insert(Wakeup(event, secs));
   return item.second;
}

//------------------------------------------------------------------------------

fn_name BotThread_QueueMsg = "BotThread.QueueMsg";

void BotThread::QueueMsg(DipIpBufferPtr& buff)
{
   Debug::ft(BotThread_QueueMsg);

   EnqMsg(*buff.release());
}
}
