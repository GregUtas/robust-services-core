//==============================================================================
//
//  BotThread.h
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
#ifndef BOTTHREAD_H_INCLUDED
#define BOTTHREAD_H_INCLUDED

#include "Thread.h"
#include <set>
#include "Clock.h"
#include "DipTypes.h"
#include "NbTypes.h"

namespace Diplomacy
{
   class BaseBot;
}

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  Thread for Diplomacy bot.
//
class BotThread : public Thread
{
   friend class Singleton< BotThread >;
public:
   //  Queues BUFF for processing.  It must begin with the DipHeader
   //  defined in DipProtocol.h.
   //
   void QueueMsg(DipIpBufferPtr& buff);

   //  Invoked when the client wants to receive a BM_Message that
   //  contains EVENT in SECS seconds.  Returns false if there is
   //  already an instance of the same event set to expire at the
   //  same time.
   //
   bool QueueEvent(BotEvent event, secs_t secs);

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
   //  Private because this singleton is not subclassed.
   //
   BotThread();

   //  Private because this singleton is not subclassed.
   //
   ~BotThread() = default;

   //  Processes an incoming message.
   //
   void ProcessMsg(MsgBuffer* msg) const;

   //  Injects an event that was to be processed after a delay.
   //
   void ProcessEvent();

   //  Overridden to return a name for the thread.
   //
   const char* AbbrName() const override;

   //  Overridden to dequeue log requests.
   //
   void Enter() override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  An event that will occur after a delay.
   //
   struct Wakeup
   {
      BotEvent event;       // event
      mutable secs_t secs;  // delay

      Wakeup(BotEvent e, secs_t s) : event(e), secs(s) { }

      bool operator<(const Wakeup& that) const
      {
         if(this->secs < that.secs) return true;
         if(this->secs > that.secs) return false;
         if(this->event < that.event) return true;
         if(this->event > that.event) return false;
         return(this < &that);
      }
   };

   //  The Diplomacy bot.
   //
   BaseBot* bot_;

   //  Set when the bot has exited.  The thread will also exit.
   //
   bool exit_;

   //  The set of pending events.
   //
   std::set< Wakeup > wakeups_;
};
}
#endif
