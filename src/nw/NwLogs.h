//==============================================================================
//
//  NwLogs.h
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
#ifndef NWLOGS_H_INCLUDED
#define NWLOGS_H_INCLUDED

#include <string>
#include "NbTypes.h"
#include "NwTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Generates a network log.  ID specifies the type of log, FUNC identifies
//  the function that failed, and ERRVAL is a platform-specific failure code.
//  If EXTRA is provided, it is appended to the log; it must include a leading
//  space or endline and Log::Tab to set it off from the rest of the log.
//
void OutputNwLog(NodeBase::LogId id, NodeBase::c_string func,
   nwerr_t errval, NodeBase::c_string extra = NodeBase::EMPTY_STR);

//  Invoked when a socket successfully sends or receives a message so that
//  if the network was out of service, the associated alarm can be cleared.
//
void NetworkIsUp();

//  Reports the result of SysSocket::StartLayer.  Returns false if ERR is
//  not empty, which indicates that StartLayer failed.
//
bool ReportLayerStart(const std::string& err);

//  Logs for NetworkBase.
//
extern NodeBase::fixed_string NetworkLogGroup;
constexpr NodeBase::LogId NetworkStartupFailure = NodeBase::TroubleLog;
constexpr NodeBase::LogId NetworkShutdownFailure = NodeBase::TroubleLog + 1;
constexpr NodeBase::LogId NetworkUnavailable = NodeBase::TroubleLog + 2;
constexpr NodeBase::LogId NetworkPortOccupied = NodeBase::TroubleLog + 3;
constexpr NodeBase::LogId NetworkServiceFailure = NodeBase::TroubleLog + 4;
constexpr NodeBase::LogId NetworkAllocFailure = NodeBase::TroubleLog + 5;
constexpr NodeBase::LogId NetworkFunctionError = NodeBase::TroubleLog + 6;
constexpr NodeBase::LogId NetworkLocalAddrFailure = NodeBase::TroubleLog + 7;
constexpr NodeBase::LogId NetworkAvailable = NodeBase::InfoLog;
constexpr NodeBase::LogId NetworkServiceAvailable = NodeBase::InfoLog + 1;
constexpr NodeBase::LogId NetworkStartupSuccess = NodeBase::InfoLog + 2;
constexpr NodeBase::LogId NetworkLocalAddrSuccess = NodeBase::InfoLog + 3;
constexpr NodeBase::LogId NetworkSocketError = NodeBase::DebugLog;
constexpr NodeBase::LogId NetworkNoDestination = NodeBase::DebugLog + 1;

extern NodeBase::fixed_string NetInitAlarmName;
extern NodeBase::fixed_string LocAddrAlarmName;
extern NodeBase::fixed_string NetworkAlarmName;

void CreateNwLogs(NodeBase::RestartLevel level);
}
#endif
