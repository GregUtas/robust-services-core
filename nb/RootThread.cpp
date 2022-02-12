//==============================================================================
//
//  RootThread.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "RootThread.h"
#include <cstdint>
#include <sstream>
#include <string>
#include "Debug.h"
#include "Duration.h"
#include "Formatters.h"
#include "InitFlags.h"
#include "InitThread.h"
#include "Log.h"
#include "LogBufferRegistry.h"
#include "NbAppIds.h"
#include "NbLogs.h"
#include "NbSignals.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysThread.h"
#include "SysThreadStack.h"
#include "ThreadAdmin.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
RootThread::RootThread() : Thread(WatchdogFaction),
   state_(Initializing)
{
   Debug::ft("RootThread.ctor");

   SetInitialized();
}

//------------------------------------------------------------------------------

RootThread::~RootThread()
{
   Debug::ftnt("RootThread.dtor");
}

//------------------------------------------------------------------------------

c_string RootThread::AbbrName() const
{
   return "root";
}

//------------------------------------------------------------------------------

void RootThread::Destroy()
{
   Debug::ft("RootThread.Destroy");

   Singleton< RootThread >::Destroy();
}

//------------------------------------------------------------------------------

void RootThread::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Thread::Display(stream, prefix, options);

   stream << prefix << "state : " << state_ << CRLF;
}

//------------------------------------------------------------------------------

void RootThread::Enter()
{
   Debug::ft("RootThread.Enter");

   Thread* initThr;
   auto timeout = TIMEOUT_IMMED;
   auto reason = NilRestart;

   //  When a thread is entered, it is unpreemptable.  However, we must run
   //  preemptably so that we don't wait for other unpreemptable threads to
   //  yield.  Our high priority ensures that we will run whenever we want.
   //
   MakePreemptable();

   while(true)
   {
      switch(state_)
      {
      case Initializing:
         //
         //  Create InitThread and then sleep to give it time to initialize the
         //  system.  When it's finished, it wakes us up.  If our timer expires,
         //  initialization failed.
         //
         Singleton< InitThread >::Instance();

         //  The following suspends RootThread during breakpoint debugging,
         //  where it would otherwise appear with annoying regularity.
         //
         if(InitFlags::SuspendRoot() || Debug::SwFlagOn(DisableRootThread))
         {
            systhrd_->Wait();
         }

         timeout = ThreadAdmin::InitTimeout();

         switch(Pause(timeout))
         {
         case DelayInterrupted:
            if(Test(InitThread::Restart))
               Reset(InitThread::Restart);
            else
               state_ = Running;
            reason = NilRestart;
            break;

         case DelayCompleted:
            reason = RestartTimeout;
            break;

         case DelayError:
         default:
            reason = ThreadPauseFailed;
         }

         //  If initialization timed out, generate a log.  If breakpointing
         //  is enabled, enter the Running state, else shut down InitThread
         //  and loop around to try again.
         //
         if(reason != NilRestart)
         {
            auto log = Log::Create(NodeLogGroup, NodeInitTimeout);

            if(log != nullptr)
            {
               *log << Log::Tab << "reason=" << strHex(uint32_t(reason));
               *log << " timeout=" << timeout.to_str(mSECS);
               Log::Submit(log);
            }

            reason = NilRestart;

            if(ThreadAdmin::ReinitOnSchedTimeout() && !InitFlags::AllowBreak())
            {
               initThr = Singleton< InitThread >::Extant();

               if(initThr != nullptr)
               {
                  initThr->Raise(SIGCLOSE);
                  Pause(Duration(100, mSECS));
               }
            }
            else
            {
               state_ = Running;
            }
         }
         break;

      case Running:
         //
         //  The following suspends RootThread during breakpoint debugging,
         //  where it would otherwise appear with annoying regularity.
         //
         if(Debug::SwFlagOn(DisableRootThread))
         {
            systhrd_->Wait();
         }

         //  The system initialized.  Sleep for the scheduling timeout.
         //
         timeout = ThreadAdmin::SchedTimeout();

         switch(Pause(timeout))
         {
         case DelayInterrupted:
            //
            //  This is usually a heartbeat from InitThread.  But it also
            //  occurs when InitThread is initiating a restart, in which
            //  case we must update our state and start to run a watchdog
            //  timer on the restart.
            //
            if(Test(InitThread::Restart))
            {
               Reset(InitThread::Restart);
               state_ = Initializing;
            }
            reason = NilRestart;
            break;

         case DelayCompleted:
            //
            //  InitThread failed to respond.  Ignore this if breakpoint
            //  debugging is enabled.
            //
            reason = SchedulingTimeout;
            if(ThreadAdmin::BreakEnabled()) reason = NilRestart;
            break;

         case DelayError:
         default:
            reason = ThreadPauseFailed;
         }

         if(reason == NilRestart) break;

         //  Continue into the default clause to initiate a restart.  We also
         //  end up there if our state somehow gets corrupted.  Note that if
         //  InitThread interrupts us to initiate a restart, we loop back to
         //  the Initializing state.
         //
         //  [[fallthrough]]
      default:
         //  A restart is necessary.  Generate a log.  If InitThread still
         //  exists, tell it to initiate a restart.  If it doesn't exist,
         //  loop around and create it.
         //
         auto log = Log::Create(NodeLogGroup, NodeSchedTimeout);

         if(log != nullptr)
         {
            *log << Log::Tab << "reason=" << strHex(uint32_t(reason));
            *log << " timeout=" << timeout.to_str(mSECS);
            Log::Submit(log);
         }

         reason = NilRestart;

         if(ThreadAdmin::ReinitOnSchedTimeout() && !InitFlags::AllowBreak())
         {
            initThr = Singleton< InitThread >::Extant();

            if(initThr != nullptr)
            {
               initThr->Interrupt(InitThread::RestartMask);
            }

            state_ = Initializing;
         }
      }
   }
}

//------------------------------------------------------------------------------

main_t RootThread::Main()
{
   Debug::ft("RootThread.Main");

   //  This loop is hypothetical because Start() does not return.
   //  If Enter() above returned, the loop would come into play.
   //
   while(true)
   {
      //  Load symbol information.
      //
      SysThreadStack::Startup(RestartReboot);

      //  Create the POSIX signals.  They are needed now so that
      //  RootThread can register for signals when it is wrapped.
      //
      CreatePosixSignals();

      //  Create the log buffer, which is used to log the progress
      //  of initialization.
      //
      Singleton< LogBufferRegistry >::Instance();

      //  Wrap the root thread and enter it.
      //
      auto root = Singleton< RootThread >::Instance();
      Thread::EnterThread(root);
   }
}

//------------------------------------------------------------------------------

void RootThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}
}
