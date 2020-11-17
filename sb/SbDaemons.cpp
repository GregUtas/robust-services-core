//==============================================================================
//
//  SbDaemons.cpp
//
//  Copyright (C) 2013-2020  Greg Utas
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

InvokerDaemon::InvokerDaemon(Faction faction, size_t size) :
   Daemon(MakeName(faction).c_str(), 1),
   faction_(faction)
{
   Debug::ft("InvokerDaemon.ctor");
}

//------------------------------------------------------------------------------

InvokerDaemon::~InvokerDaemon()
{
   Debug::ftnt("InvokerDaemon.dtor");
}

//------------------------------------------------------------------------------

Thread* InvokerDaemon::CreateThread()
{
   Debug::ft("InvokerDaemon.CreateThread");

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

AlarmStatus InvokerDaemon::GetAlarmLevel() const
{
   Debug::ft("InvokerDaemon.GetAlarmLevel");

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

InvokerDaemon* InvokerDaemon::GetDaemon(Faction faction, size_t size)
{
   Debug::ft("InvokerDaemon.GetDaemon");

   auto reg = Singleton< DaemonRegistry >::Instance();
   auto name = MakeName(faction);
   auto daemon = static_cast< InvokerDaemon* >(reg->FindDaemon(name.c_str()));

   if(daemon != nullptr) return daemon;
   return new InvokerDaemon(faction, size);
}

//------------------------------------------------------------------------------

string InvokerDaemon::MakeName(Faction faction)
{
   Debug::ft("InvokerDaemon.MakeName");

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

TimerDaemon::TimerDaemon() : Daemon(TimerDaemonName, 1)
{
   Debug::ft("TimerDaemon.ctor");
}

//------------------------------------------------------------------------------

TimerDaemon::~TimerDaemon()
{
   Debug::ftnt("TimerDaemon.dtor");
}

//------------------------------------------------------------------------------

Thread* TimerDaemon::CreateThread()
{
   Debug::ft("TimerDaemon.CreateThread");

   return Singleton< TimerThread >::Instance();
}

//------------------------------------------------------------------------------

AlarmStatus TimerDaemon::GetAlarmLevel() const
{
   Debug::ft("TimerDaemon.GetAlarmLevel");

   return CriticalAlarm;
}

//------------------------------------------------------------------------------

void TimerDaemon::Patch(sel_t selector, void* arguments)
{
   Daemon::Patch(selector, arguments);
}
}
