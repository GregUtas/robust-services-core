//==============================================================================
//
//  RootThread.h
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
#ifndef ROOTTHREAD_H_INCLUDED
#define ROOTTHREAD_H_INCLUDED

#include "Thread.h"
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  The thread created to run main() quickly creates RootThread, which
//  is responsible for
//  o creating InitThread and the minimal set of objects required for
//    InitThread to finish initializing or restarting the system,
//  o ensuring that initialization or a restart succeeds before a timeout,
//  o ensuring that InitThread is running while the system is in service,
//  o causing the executable to exit when a reboot or exit is requested.
//
class RootThread : public Thread
{
   friend class Singleton< RootThread >;
   friend class InitThread;
public:
   //  Invoked as the only line of code in main().
   //
   static main_t Main();

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
   RootThread();

   //  Private because this is a singleton.
   //
   ~RootThread();

   //  Overridden to return a name for the thread.
   //
   c_string AbbrName() const override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  Overridden to create InitThread, to ensure that InitThread finishes
   //  initializing the system, and to ensure that InitThread subsequently
   //  runs periodically.
   //
   void Enter() override;

   //  States for the root thread.
   //
   enum State
   {
      Initializing,  // system being initialized
      Running        // system is in service
   };

   //  The thread's current state.
   //
   State state_;
};
}
#endif
