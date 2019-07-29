//==============================================================================
//
//  NwLogs.h
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
#ifndef NWLOGS_H_INCLUDED
#define NWLOGS_H_INCLUDED

#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
   //  Logs for NetworkBase.
   //
   extern NodeBase::fixed_string NetworkLogGroup;
   constexpr NodeBase::LogId NetworkStartupFailure = NodeBase::TroubleLog;
   constexpr NodeBase::LogId NetworkShutdownFailure = NodeBase::TroubleLog + 1;
   constexpr NodeBase::LogId NetworkUnavailable = NodeBase::TroubleLog + 2;
   constexpr NodeBase::LogId NetworkPortOccupied = NodeBase::TroubleLog + 3;
   constexpr NodeBase::LogId NetworkIoThreadFailure = NodeBase::TroubleLog + 4;
   constexpr NodeBase::LogId NetworkAllocFailure = NodeBase::TroubleLog + 5;
   constexpr NodeBase::LogId NetworkFunctionError = NodeBase::TroubleLog + 6;
   constexpr NodeBase::LogId NetworkAvailable = NodeBase::InfoLog;
   constexpr NodeBase::LogId NetworkSocketError = NodeBase::DebugLog;
   constexpr NodeBase::LogId NetworkNoDestination = NodeBase::DebugLog + 1;

   extern NodeBase::fixed_string NetworkAlarmName;

   void CreateNwLogs(NodeBase::RestartLevel level);
}
#endif