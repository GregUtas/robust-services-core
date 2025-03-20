//==============================================================================
//
//  DeferredThread.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "DeferredThread.h"
#include <ratio>
#include "Debug.h"
#include "DeferredRegistry.h"
#include "Duration.h"
#include "NbDaemons.h"
#include "Singleton.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
DeferredThread::DeferredThread() :
   Thread(MaintenanceFaction, Singleton<DeferredDaemon>::Instance())
{
   Debug::ft("DeferredThread.ctor");

   SetInitialized();
}

//------------------------------------------------------------------------------

DeferredThread::~DeferredThread()
{
   Debug::ftnt("DeferredThread.dtor");
}

//------------------------------------------------------------------------------

c_string DeferredThread::AbbrName() const
{
   return DeferredDaemonName;
}

//------------------------------------------------------------------------------

void DeferredThread::Destroy()
{
   Debug::ft("DeferredThread.Destroy");

   Singleton<DeferredThread>::Destroy();
}

//------------------------------------------------------------------------------

void DeferredThread::Enter()
{
   Debug::ft("DeferredThread.Enter");

   //  Every second, tell our registry to notify items whose timeout has
   //  occurred.
   //
   auto reg = Singleton<DeferredRegistry>::Instance();
   msecs_t sleep(msecs_t(1 * ONE_SEC));

   while(true)
   {
      Pause(sleep);
      reg->RaiseTimeouts();

      //  Sleep for one second, minus the amount of time that we just ran.
      //
      auto runTime = CurrTimeRunning();
      sleep = (runTime > secs_t(1) ?
         TIMEOUT_IMMED : msecs_t(1 * ONE_SEC) - runTime);
   }
}

//------------------------------------------------------------------------------

void DeferredThread::Patch(sel_t selector, void* arguments)
{
   Thread::Patch(selector, arguments);
}
}
