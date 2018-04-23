//==============================================================================
//
//  Restart.h
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
#ifndef RESTART_H_INCLUDED
#define RESTART_H_INCLUDED

#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The element's state.
//
enum RestartStatus
{
   Initial,      // system was just booted
   StartingUp,   // system is being initialized
   Running,      // system is in operation
   ShuttingDown  // system is being shut down
};

//------------------------------------------------------------------------------
//
//  Reasons for shutdowns/restarts.
//
//  Each user of Initiate (below) must define a value here.
//
enum RestartReasons
{
   NilRestart             = 0x0000,  // nil value
   ManualRestart          = 0x0001,  // CLI >restart command
   SystemOutOfMemory      = 0x0010,  // memory exhausted
   ModuleStartupFailed    = 0x0020,  // failed to allocate resources
   SocketLayerUnavailable = 0x0030,  // socket layer could not be started
   RestartTimeout         = 0x0040,  // restart took too long
   SchedulingTimeout      = 0x0041,  // missed InitThread heartbeat
   ThreadPauseFailed      = 0x0050,  // Thread::Pause failed
   DeathOfCriticalThread  = 0x0051,  // irrecoverable exception
   WorkQueueCorruption    = 0x0100,  // corrupt invoker work queue
   TimerQueueCorruption   = 0x0101   // corrupt timer registry queue
};

//------------------------------------------------------------------------------

class Restart
{
   friend class ModuleRegistry;
public:
   //  Returns the system's initialization status.
   //
   static RestartStatus GetStatus() { return Status_; }

   //  Returns the type of restart currently in progress.
   //
   static RestartLevel GetLevel() { return Level_; }

   //  Forces a restart after generating a log.  REASON must be defined
   //  above and indicates why the restart was initiated.  ERRVAL is for
   //  debugging.
   //
   static void Initiate(reinit_t reason, debug32_t errval);
private:
   //  Deleted because this class only has static members.
   //
   Restart() = delete;

   //  The state of system initialization or shutdown.
   //
   static RestartStatus Status_;

   //  The type of initialization or shutdown being performed.
   //
   static RestartLevel Level_;
};
}
#endif