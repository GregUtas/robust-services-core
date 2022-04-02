//==============================================================================
//
//  AnModule.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "AnModule.h"
#include "AnIncrement.h"
#include "Debug.h"
#include "ModuleRegistry.h"
#include "PbModule.h"
#include "PotsShelf.h"
#include "Singleton.h"

using namespace PotsBase;

//------------------------------------------------------------------------------

namespace AccessNode
{
AnModule::AnModule() : Module()
{
   Debug::ft("AnModule.ctor");

   //  Create the modules required by AccessNode.
   //
   Singleton< PbModule >::Instance();
   Singleton< ModuleRegistry >::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

AnModule::~AnModule()
{
   Debug::ftnt("AnModule.dtor");
}

//------------------------------------------------------------------------------

void AnModule::Shutdown(RestartLevel level)
{
   Debug::ft("AnModule.Shutdown");
}

//------------------------------------------------------------------------------

void AnModule::Startup(RestartLevel level)
{
   Debug::ft("AnModule.Startup");

   Singleton< PotsShelfFactory >::Instance()->Startup(level);
   Singleton< PotsShelfIpService >::Instance()->Startup(level);
   Singleton< AnIncrement >::Instance()->Startup(level);
}
}
