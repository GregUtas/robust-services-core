//==============================================================================
//
//  BotThread.h
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
#ifndef BOTTHREAD_H_INCLUDED
#define BOTTHREAD_H_INCLUDED

#include "Thread.h"
#include <cstdint>
#include <list>
#include "DipTypes.h"
#include "NbTypes.h"
#include "SteadyTime.h"

namespace Diplomacy
{
   class BaseBot;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  An event that will occur after a delay.
//
struct BotWakeup
{
   //  Creates an item that will raise EVENT in SECS.
   //
   BotWakeup(BotEvent event, uint32_t secs);

   //  Returns the number of milliseconds until the wakeup will occur.
   //  Returns 0 if the event is past due.
   //
   uint64_t TimeToExpiry() const;

   //  The event.
   //
   BotEvent event_;

   //  The time when the event will occur.
   //
   SteadyTime::Point expiry_;
};

//------------------------------------------------------------------------------
//
//  Thread for Diplomacy bot.
//
class BotThread : public Thread
{
   friend class Singleton<BotThread>;
public:
   //  Queues BUFF for processing.  It must begin with the DipHeader
   //  defined in DipProtocol.h.
   //
   void QueueMsg(DipIpBufferPtr& buff);

   //  Invoked when the client wants to receive a BM_Message that
   //  contains EVENT in SECS seconds.
   //
   void QueueEvent(BotEvent event, uint32_t secs);

   //  Cancels EVENT if it exists.  If more than one such event is
   //  pending, only the one that would occur first is cancelled.
   //
   void CancelEvent(BotEvent event);

   //  Sets a flag that tells the thread that the bot has exited.
   //
   void SetExit() { exit_ = true; }

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   BotThread();

   //  Private because this is a singleton.
   //
   ~BotThread() = default;

   //  Processes an incoming message.
   //
   void ProcessMsg(MsgBuffer* msg) const;

   //  Injects the event(s) to be processed after a delay.
   //
   void ProcessEvents();

   //  Overridden to return a name for the thread.
   //
   c_string AbbrName() const override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  Overridden to dequeue log requests.
   //
   void Enter() override;

   //  The Diplomacy bot.
   //
   BaseBot* bot_;

   //  Set when the bot has exited.  The thread will also exit.
   //
   bool exit_;

   //  Pending events.
   //
   std::list<BotWakeup> wakeups_;
};
}
#endif
