//==============================================================================
//
//  NbDaemons.h
//
//  Copyright (C) 2013-2021  Greg Utas
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
#ifndef NBDAEMONS_H_INCLUDED
#define NBDAEMONS_H_INCLUDED

#include "Daemon.h"
#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  Daemons for managing NodeBase threads.  The following threads do not have
//  daemons because they will be recreated, on demand, by Singleton.Instance:
//  o CinThread (by CinThread.GetLine)
//  o CoutThread (by CoutThread.Spool)
//  o FileThread (by FileThread.Spool)
//  o InitThread (by Thread.Ready, Thread.Schedule, and others)
//
extern fixed_string CliDaemonName;

class CliDaemon : public Daemon
{
   friend class Singleton< CliDaemon >;
public:
   void Patch(sel_t selector, void* arguments) override;
private:
   CliDaemon();
   ~CliDaemon();
   Thread* CreateThread() override;
   AlarmStatus GetAlarmLevel() const override;
};

//------------------------------------------------------------------------------

extern fixed_string LogDaemonName;

class LogDaemon : public Daemon
{
   friend class Singleton< LogDaemon >;
public:
   void Patch(sel_t selector, void* arguments) override;
private:
   LogDaemon();
   ~LogDaemon();
   Thread* CreateThread() override;
   AlarmStatus GetAlarmLevel() const override;
};

//------------------------------------------------------------------------------

extern fixed_string ObjectDaemonName;

class ObjectDaemon : public Daemon
{
   friend class Singleton< ObjectDaemon >;
public:
   void Patch(sel_t selector, void* arguments) override;
private:
   ObjectDaemon();
   ~ObjectDaemon();
   Thread* CreateThread() override;
};

//------------------------------------------------------------------------------

extern fixed_string StatisticsDaemonName;

class StatisticsDaemon : public Daemon
{
   friend class Singleton< StatisticsDaemon >;
public:
   void Patch(sel_t selector, void* arguments) override;
private:
   StatisticsDaemon();
   ~StatisticsDaemon();
   Thread* CreateThread() override;
};
}
#endif
