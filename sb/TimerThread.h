//==============================================================================
//
//  TimerThread.h
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
#ifndef TIMERTHREAD_H_INCLUDED
#define TIMERTHREAD_H_INCLUDED

#include "Thread.h"
#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Services the timer queues (in TimerRegistry) by sending timeout messages
//  to timers that have expired.
//
class TimerThread : public NodeBase::Thread
{
   friend class NodeBase::Singleton< TimerThread >;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this singleton is not subclassed.
   //
   TimerThread();

   //  Overridden to return a name for the thread.
   //
   NodeBase::c_string AbbrName() const override;

   //  Private because this singleton is not subclassed.
   //
   ~TimerThread();

   //  Overridden to support excluding or including all timer threads.
   //
   NodeBase::TraceStatus CalcStatus(bool dynamic) const override;

   //  Overridden to enter a loop that tells the timer registry, once per
   //  second, to send timeout messages on behalf of expired timers.
   //
   void Enter() override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;
};
}
#endif
