//==============================================================================
//
//  InitThread.h
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
#ifndef INITTHREAD_H_INCLUDED
#define INITTHREAD_H_INCLUDED

#include "Thread.h"
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  This thread is first one created by RootThread and is responsible for
//  o initializing the system
//  o restarting the system
//  o invoking Daemons so they can recreate threads that have exited
//  o enforcing the run-to-completion timeout
//  o initiating context switches
//
class InitThread : public Thread
{
   friend class Singleton< InitThread >;
   friend class Daemon;
   friend class RootThread;
   friend class Thread;
public:
   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  States for the initialization thread.
   //
   enum State
   {
      Initializing,  // system is being initialized or restarted
      Running,       // system is in service
      Restarting     // internal error: initiating a restart
   };

   //  Flags used when interrupting InitThread.
   //
   static const FlagId Restart = 0;   // restart system; also used by RootThread
   static const FlagId Recreate = 1;  // recreate critical thread
   static const FlagId Schedule = 2;  // schedule next thread

   //  The mask passed to Thread::Interrupt to set one of the above flags.
   //
   static const Flags RestartMask;
   static const Flags RecreateMask;
   static const Flags ScheduleMask;

   //  Private because this singleton is not subclassed.
   //
   InitThread();

   //  Private because this singleton is not subclassed.
   //
   ~InitThread();

   //  Initiates a restart at LEVEL.
   //
   void InitiateRestart(RestartLevel level);

   //  Initializes or restarts the system.
   //
   void InitializeSystem();

   //  Calculates the run-to-completion timeout (our sleep interval).
   //
   Duration CalculateDelay() const;

   //  Invoked after sleeping for the expected duration.
   //
   void HandleTimeout();

   //  Invoked if interrupted while sleeping.
   //
   void HandleInterrupt();

   //  Initiates a restart when a critical thread cannot be recreated.
   //
   void CauseRestart();

   //  Recreates any critical threads that have exited.
   //
   void RecreateThreads();

   //  Initiates a context switch.
   //
   void ContextSwitch();

   //  Overridden to return a name for the thread.
   //
   c_string AbbrName() const override;

   //  Overridden to initialize the system and then run in the background
   //  to enforce the run-to-completion timeout, initiate context switches,
   //  and recreate application threads.
   //
   void Enter() override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  An error value for debugging.
   //
   debug64_t errval_;

   //  The thread's current state.
   //
   State state_;

   //  Set when a run-to-completion timeout has occurred.
   //
   bool timeout_;
};
}
#endif
