//==============================================================================
//
//  SbLogs.h
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
#ifndef SBLOGS_H_INCLUDED
#define SBLOGS_H_INCLUDED

#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
   //  Logs for SessionBase.
   //
   extern NodeBase::fixed_string SessionLogGroup;
   constexpr NodeBase::LogId InvokerPoolBlocked = NodeBase::TroubleLog;
   constexpr NodeBase::LogId SessionOverload = NodeBase::ThresholdLog;
   constexpr NodeBase::LogId SessionNoOverload = NodeBase::InfoLog;
   constexpr NodeBase::LogId SessionError = NodeBase::DebugLog;
   constexpr NodeBase::LogId ServiceError = NodeBase::DebugLog + 1;
   constexpr NodeBase::LogId InvokerWorkQueueCount = NodeBase::DebugLog + 2;
   constexpr NodeBase::LogId InvokerDiscardedBuffer = NodeBase::DebugLog + 3;
   constexpr NodeBase::LogId InvokerDiscardedMessage = NodeBase::DebugLog + 4;
   constexpr NodeBase::LogId InvalidIncomingMessage = NodeBase::DebugLog + 5;

   extern NodeBase::fixed_string OverloadAlarmName;

   void CreateSbLogs(NodeBase::RestartLevel level);
}
#endif
