//==============================================================================
//
//  SbDaemons.h
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
#ifndef SBDAEMONS_H_INCLUDED
#define SBDAEMONS_H_INCLUDED

#include "Daemon.h"
#include <cstddef>
#include <string>
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Daemons for managing SessionBase threads.
//
extern NodeBase::fixed_string InvokerDaemonName;

class InvokerDaemon : public NodeBase::Daemon
{
public:
   //  Finds/creates the daemon that manages SIZE invoker threads in FACTION.
   //
   static InvokerDaemon* GetDaemon(NodeBase::Faction faction, size_t size);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;
private:
   //  Creates a daemon that will manage SIZE invoker threads in FACTION.
   //
   InvokerDaemon(NodeBase::Faction faction, size_t size);

   //  Not subclassed.
   //
   ~InvokerDaemon();

   //  Returns the name for the daemon in FACTION.
   //
   static std::string MakeName(NodeBase::Faction faction);

   //  Overridden to create an invoker thread in the appropriate faction.
   //
   NodeBase::Thread* CreateThread() override;

   //  Overridden to return an alarm based on the percentage of available
   //  invoker threads.
   //
   NodeBase::AlarmStatus GetAlarmLevel() const override;

   //  The faction for the invoker threads.
   //
   const NodeBase::Faction faction_;
};

//------------------------------------------------------------------------------

extern NodeBase::fixed_string TimerDaemonName;

class TimerDaemon : public NodeBase::Daemon
{
   friend class NodeBase::Singleton< TimerDaemon >;
private:
   TimerDaemon();
   ~TimerDaemon();
   NodeBase::Thread* CreateThread() override;
   NodeBase::AlarmStatus GetAlarmLevel() const override;
};
}
#endif
