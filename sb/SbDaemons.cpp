//==============================================================================
//
//  SbDaemons.cpp
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
#include "SbDaemons.h"
#include <ostream>
#include <set>
#include "DaemonRegistry.h"
#include "Debug.h"
#include "InvokerThread.h"
#include "Singleton.h"
#include "TimerThread.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fixed_string InvokerDaemonName = "invoker";

//------------------------------------------------------------------------------

fn_name InvokerDaemon_ctor = "InvokerDaemon.ctor";

InvokerDaemon::InvokerDaemon(Faction faction, size_t size) :
   Daemon(MakeName(faction).c_str(), 1),
   faction_(faction)
{
   Debug::ft(InvokerDaemon_ctor);
}

//------------------------------------------------------------------------------

fn_name InvokerDaemon_dtor = "InvokerDaemon.dtor";

InvokerDaemon::~InvokerDaemon()
{
   Debug::ft(InvokerDaemon_dtor);
}

//------------------------------------------------------------------------------

fn_name InvokerDaemon_CreateThread = "InvokerDaemon.CreateThread";

Thread* InvokerDaemon::CreateThread()
{
   Debug::ft(InvokerDaemon_CreateThread);

   return new InvokerThread(faction_, this);
}

//------------------------------------------------------------------------------

void InvokerDaemon::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Daemon::Display(stream, prefix, options);

   stream << prefix << "faction : " << faction_ << CRLF;
}

//------------------------------------------------------------------------------

fn_name InvokerDaemon_GetAlarmLevel = "InvokerDaemon.GetAlarmLevel";

AlarmStatus InvokerDaemon::GetAlarmLevel() const
{
   Debug::ft(InvokerDaemon_GetAlarmLevel);

   //  Anything other than a critical alarm is rather hypothetical because
   //  there should have been enough traps to cause a restart if multiple
   //  invoker threads could not be recreated after being forced to exit.
   //
   auto percent = 100 * Threads().size() / TargetSize();
   if(percent <= 25) return CriticalAlarm;
   if(percent <= 50) return MajorAlarm;
   return MinorAlarm;
}

//------------------------------------------------------------------------------

fn_name InvokerDaemon_GetDaemon = "InvokerDaemon.GetDaemon";

InvokerDaemon* InvokerDaemon::GetDaemon(Faction faction, size_t size)
{
   Debug::ft(InvokerDaemon_GetDaemon);

   auto reg = Singleton< DaemonRegistry >::Instance();
   auto name = MakeName(faction);
   auto daemon = static_cast< InvokerDaemon* >(reg->FindDaemon(name.c_str()));

   if(daemon == nullptr)
   {
      daemon = new InvokerDaemon(faction, size);
   }

   return daemon;
}

//------------------------------------------------------------------------------

fn_name InvokerDaemon_MakeName = "InvokerDaemon.MakeName";

string InvokerDaemon::MakeName(Faction faction)
{
   Debug::ft(InvokerDaemon_MakeName);

   //  A Daemon requires a unique name, so append the faction's character
   //  to the basic name.
   //
   string name(InvokerDaemonName);
   name.push_back('_');
   name.push_back(FactionChar(faction));
   return name;
}

//------------------------------------------------------------------------------

void InvokerDaemon::Patch(sel_t selector, void* arguments)
{
   Daemon::Patch(selector, arguments);
}

//==============================================================================

fixed_string TimerDaemonName = "timer";

//------------------------------------------------------------------------------

fn_name TimerDaemon_ctor = "TimerDaemon.ctor";

TimerDaemon::TimerDaemon() : Daemon(TimerDaemonName, 1)
{
   Debug::ft(TimerDaemon_ctor);
}

//------------------------------------------------------------------------------

fn_name TimerDaemon_dtor = "TimerDaemon.dtor";

TimerDaemon::~TimerDaemon()
{
   Debug::ft(TimerDaemon_dtor);
}

//------------------------------------------------------------------------------

fixed_string TimerDaemon_CreateThread = "TimerDaemon.CreateThread";

Thread* TimerDaemon::CreateThread()
{
   Debug::ft(TimerDaemon_CreateThread);

   return Singleton< TimerThread >::Instance();
}

//------------------------------------------------------------------------------

fn_name TimerDaemon_GetAlarmLevel = "TimerDaemon.GetAlarmLevel";

AlarmStatus TimerDaemon::GetAlarmLevel() const
{
   Debug::ft(TimerDaemon_GetAlarmLevel);

   return CriticalAlarm;
}

//------------------------------------------------------------------------------

void TimerDaemon::Patch(sel_t selector, void* arguments)
{
   Daemon::Patch(selector, arguments);
}
}
