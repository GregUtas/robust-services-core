//==============================================================================
//
//  PotsLogs.cpp
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
#include "PotsLogs.h"
#include "Debug.h"
#include "Log.h"
#include "LogGroup.h"

using namespace NodeBase;

//------------------------------------------------------------------------------

namespace PotsBase
{
fixed_string PotsLogGroup = "POTS";

//------------------------------------------------------------------------------

void CreatePotsLogs(RestartLevel level)
{
   Debug::ft("PotsBase.CreatePotsLogs");

   if(level < RestartReboot) return;

   auto group = new LogGroup(PotsLogGroup, "POTS Application");
   new Log(group, PotsShelfIcSignal, "POTS shelf invalid incoming signal");
   new Log(group, PotsShelfIcBuffer, "POTS shelf invalid incoming buffer");
   new Log(group, PotsShelfIcMessage, "POTS shelf invalid incoming message");
   new Log(group, PotsCallIcSignal, "POTS call invalid incoming signal");
   new Log(group, PotsCallIcBuffer, "POTS call invalid incoming buffer");
   new Log(group, PotsShelfCircuitReset, "POTS shelf circuit reset");
   new Log(group, PotsTrafficRate, "POTS traffic rate");
   new Log(group, PotsShelfOgSignal, "POTS shelf invalid outgoing signal");
}
}
