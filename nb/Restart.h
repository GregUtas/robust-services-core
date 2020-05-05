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

#include <memory>
#include "Base.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The current restart stage.
//
enum RestartStage
{
   Launching,    // system is just booting
   StartingUp,   // system is being reinitialized
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
   NilRestart               = 0x0000,  // nil value
   ManualRestart            = 0x0001,  // CLI >restart command
   ObjectPoolCreationFailed = 0x0010,  // insufficient memory for object pool
   ModuleStartupFailed      = 0x0020,  // failed to allocate resources
   NetworkLayerUnavailable  = 0x0030,  // network layer could not be started
   RestartTimeout           = 0x0040,  // restart took too long
   SchedulingTimeout        = 0x0041,  // missed InitThread heartbeat
   ThreadPauseFailed        = 0x0050,  // Thread::Pause failed
   DeathOfCriticalThread    = 0x0051,  // irrecoverable exception
   MutexCreationFailed      = 0x0052,  // failed to create mutex
   HeapCreationFailed       = 0x0060,  // insufficient memory for heap
   HeapCorruption           = 0x0060,  // corrupt heap detected
   HeapProtection           = 0x0061,  // failed to change memory protection
   WorkQueueCorruption      = 0x0100,  // corrupt invoker work queue
   TimerQueueCorruption     = 0x0101   // corrupt timer registry queue
};

//------------------------------------------------------------------------------

class Restart
{
   friend class ModuleRegistry;
public:
   //  Deleted because this class only has static members.
   //
   Restart() = delete;

   //  Returns the system's initialization stage.
   //
   static RestartStage GetStage() { return Stage_; }

   //  Returns the type of restart currently in progress.
   //
   static RestartLevel GetLevel() { return Level_; }

   //  Returns true if the heap for memory of TYPE will be freed
   //  and reallocated during any restart that is underway.
   //
   static bool ClearsMemory(MemoryType type);

   //  Invokes obj.release() and returns true if OBJ's heap will be
   //  freed during any restart that is currently underway.
   //
   template< class T > static bool Release(std::unique_ptr< T >& obj)
   {
      auto type = (obj == nullptr ? MemNull : obj->MemType());
      if(!ClearsMemory(type)) return false;
      obj.release();
      return true;
   }

   //  Forces a restart after generating a log.  REASON must be defined
   //  above and indicates why the restart was initiated.  ERRVAL is for
   //  debugging.
   //
   static void Initiate(reinit_t reason, debug64_t errval);
private:
   //  The current stage of system initialization or shutdown.
   //
   static RestartStage Stage_;

   //  The type of initialization or shutdown being performed.
   //
   static RestartLevel Level_;
};
}
#endif
