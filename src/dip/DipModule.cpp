//==============================================================================
//
//  DipModule.cpp
//
//  Copyright (C) 2019-2022  Greg Utas
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
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
#include "DipModule.h"
#include "BotThread.h"
#include "BotTracer.h"
#include "Debug.h"
#include "DipProtocol.h"
#include "ModuleRegistry.h"
#include "NwModule.h"
#include "Singleton.h"

using namespace NodeBase;
using namespace NetworkBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
DipModule::DipModule() : Module("dip")
{
   Debug::ft("DipModule.ctor");

   //  Create the modules required by Diplomacy.
   //
   Singleton<NwModule>::Instance();
   Singleton<ModuleRegistry>::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

void DipModule::Enable()
{
   Debug::ft("DipModule.Enable");

   Singleton<NwModule>::Instance()->Enable();
   Module::Enable();
}

//------------------------------------------------------------------------------

void DipModule::Shutdown(RestartLevel level)
{
   Debug::ft("DipModule.Shutdown");
}

//------------------------------------------------------------------------------

void DipModule::Startup(RestartLevel level)
{
   Debug::ft("DipModule.Startup");

   Singleton<DipIpBufferPool>::Instance()->Startup(level);
   Singleton<BotTcpService>::Instance()->Startup(level);
   Singleton<BotTracer>::Instance();
   Singleton<BotThread>::Instance()->Startup(level);
}
}
