//==============================================================================
//
//  SbLogs.cpp
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
#include "SbLogs.h"
#include "Alarm.h"
#include "Debug.h"
#include "Log.h"
#include "LogGroup.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
fixed_string SessionLogGroup = "SESS";
fixed_string OverloadAlarmName = "OVERLOAD";

//------------------------------------------------------------------------------

fn_name SessionBase_CreateSbLogs = "SessionBase.CreateSbLogs";

void CreateSbLogs(RestartLevel level)
{
   Debug::ft(SessionBase_CreateSbLogs);

   if(level < RestartWarm) return;

   new Alarm(OverloadAlarmName, "Payload processing is overloaded", 30);

   auto group = new LogGroup(SessionLogGroup, "Sessions");
   new Log(group, InvokerPoolBlocked, "Invoker pool blocked");
   new Log(group, SessionOverload, "Payload processing now overloaded");
   new Log(group, SessionNoOverload, "Payload processing no longer overloaded");
   new Log(group, SessionError, "Session error");
   new Log(group, ServiceError, "Service error");
   new Log(group, InvokerWorkQueueCount, "Invoker work queue count incorrect");
   new Log(group, InvokerDiscardedBuffer, "Invoker discarded buffer");
   new Log(group, InvokerDiscardedMessage, "Invoker discarded message");
   new Log(group, InvalidIncomingMessage, "Invalid incoming message");
}
}