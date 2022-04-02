//==============================================================================
//
//  DeferredThread.h
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
#ifndef DEFERREDTHREAD_H_INCLUDED
#define DEFERREDTHREAD_H_INCLUDED

#include "Thread.h"
#include "NbTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Services the deferred work item queues (in DeferredRegistry) by
//  notifying items whose timers have expired.
//
class DeferredThread : public Thread
{
   friend class Singleton< DeferredThread >;
public:
   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Private because this is a singleton.
   //
   DeferredThread();

   //  Private because this is a singleton.
   //
   ~DeferredThread();

   //  Overridden to return a name for the thread.
   //
   c_string AbbrName() const override;

   //  Overridden to delete the singleton.
   //
   void Destroy() override;

   //  Overridden to enter a loop that tells the item registry, once
   //  per second, to notify items whose timers have expired.
   //
   void Enter() override;
};
}
#endif
