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

#include <iosfwd>
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
//  Reasons for restarts/shutdowns.
//
//  Each user of Initiate (below) must define a value here.
//
enum RestartReason
{
   NilRestart,                // nil value
   ManualRestart,             // CLI >restart command
   MutexCreationFailed,       // failed to create mutex
   HeapCreationFailed,        // insufficient memory for heap
   ObjectPoolCreationFailed,  // insufficient memory for object pool
   NetworkLayerUnavailable,   // network layer could not be started
   RestartTimeout,            // restart took too long
   SchedulingTimeout,         // missed InitThread heartbeat
   ThreadPauseFailed,         // Thread::Pause failed
   DeathOfCriticalThread,     // irrecoverable exception
   HeapProtectionFailed,      // failed to change memory protection
   HeapCorruption,            // corrupt heap detected
   WorkQueueCorruption,       // corrupt invoker work queue
   TimerQueueCorruption,      // corrupt timer registry queue
   RestartReason_N            // number of restart reasons
};

//  Inserts a string for REASON into STREAM.
//
std::ostream& operator<<(std::ostream& stream, RestartReason reason);

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

   //  Returns the minimum level required to destroy memory of TYPE.
   //
   static RestartLevel LevelToClear(MemoryType type);

   //  Generates a log and forces a restart at LEVEL (or higher, if escalation
   //  occurs).  REASON must be defined above and indicates why the restart was
   //  initiated.  ERRVAL is for debugging.
   //
   static void Initiate
      (RestartLevel level, RestartReason reason, debug64_t errval);
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
