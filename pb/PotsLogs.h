//==============================================================================
//
//  PotsLogs.h
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
#ifndef POTSLOGS_H_INCLUDED
#define POTSLOGS_H_INCLUDED

#include "NbTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace PotsBase
{
   //  Logs for PotsBase.
   //
   extern NodeBase::fixed_string PotsLogGroup;
   constexpr NodeBase::LogId PotsShelfIcSignal = NodeBase::TroubleLog;
   constexpr NodeBase::LogId PotsShelfIcBuffer = NodeBase::TroubleLog + 1;
   constexpr NodeBase::LogId PotsShelfIcMessage = NodeBase::TroubleLog + 2;
   constexpr NodeBase::LogId PotsCallIcSignal = NodeBase::TroubleLog + 3;
   constexpr NodeBase::LogId PotsCallIcBuffer = NodeBase::TroubleLog + 4;
   constexpr NodeBase::LogId PotsShelfCircuitReset = NodeBase::StateLog;
   constexpr NodeBase::LogId PotsTrafficRate = NodeBase::InfoLog;
   constexpr NodeBase::LogId PotsShelfOgSignal = NodeBase::DebugLog;

   void CreatePotsLogs(NodeBase::RestartLevel level);
}
#endif
