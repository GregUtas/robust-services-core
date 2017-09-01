//==============================================================================
//
//  InitThread.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef INITTHREAD_H_INCLUDED
#define INITTHREAD_H_INCLUDED

#include "Thread.h"
#include "Clock.h"
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  This thread is first one created by RootThread and is responsible for
//  o initializing and restarting the system
//  o enforcing the run-to-completion timeout when SIGVTALRM is not supported
//  o restarting suspended application threads
//
class InitThread : public Thread
{
   friend class Singleton< InitThread >;
   friend class Thread;
   friend class RootThread;
public:
   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  States for the initialization thread.
   //
   enum State
   {
      Initializing,  // system being initialized
      Running,       // system is in service
      Restarting     // initiating a restart
   };

   //  A flag used by InitThread and RootThread to indicate that
   //  a restart is being performed.
   //
   static const FlagId Restart = 0;

   //  The mask passed to Thread::Interrupt to signal a restart.
   //
   static const Flags RestartMask;

   //  Private because this singleton is not subclassed.
   //
   InitThread();

   //  Private because this singleton is not subclassed.
   //
   ~InitThread();

   //  Initiates a restart that resulted from REASON and ERRVAL.
   //
   void InitiateRestart(reinit_t reason, debug32_t errval);

   //  Initializes or restarts the system.
   //
   void InitializeSystem();

   //  Calculates the run-to-completion timeout (our sleep interval).
   //
   msecs_t CalculateDelay() const;

   //  Invoked after sleeping for the expected duration.
   //
   void HandleTimeout();

   //  Invoked when awoken prematurely.
   //
   void HandleInterrupt();

   //  Initiates a restart when a critical thread cannot be recreated.
   //
   void CauseRestart();

   //  Overridden to return a name for the thread.
   //
   virtual const char* AbbrName() const override;

   //  Overridden to initialize the system and then run in the background
   //  to enforce the run-to-completion timeout and recreate application
   //  threads.
   //
   virtual void Enter() override;

   //  Overridden to delete the singleton.
   //
   virtual void Destroy() override;

   //  The thread's current state.
   //
   State state_;

   //  Set when a run-to-completion timeout has occurred.
   //
   bool timeout_;

   //  An error value for debugging.
   //
   debug32_t errval_;
};
}
#endif
