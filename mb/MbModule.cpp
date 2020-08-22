//==============================================================================
//
//  MbModule.cpp
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
#include "MbModule.h"
#include "Debug.h"
#include "MbPools.h"
#include "ModuleRegistry.h"
#include "SbModule.h"
#include "Singleton.h"
#include "Switch.h"
#include "SymbolRegistry.h"
#include "SysTypes.h"
#include "ToneRegistry.h"
#include "Tones.h"

using namespace SessionBase;

//------------------------------------------------------------------------------

namespace MediaBase
{
fn_name MbModule_ctor = "MbModule.ctor";

MbModule::MbModule() : Module()
{
   Debug::ft(MbModule_ctor);

   //  Create the modules required by MediaBase.
   //
   Singleton< SbModule >::Instance();
   Singleton< ModuleRegistry >::Instance()->BindModule(*this);
}

//------------------------------------------------------------------------------

fn_name MbModule_dtor = "MbModule.dtor";

MbModule::~MbModule()
{
   Debug::ftnt(MbModule_dtor);
}

//------------------------------------------------------------------------------

fn_name MbModule_Shutdown = "MbModule.Shutdown";

void MbModule::Shutdown(RestartLevel level)
{
   Debug::ft(MbModule_Shutdown);

   Singleton< ToneRegistry >::Instance()->Shutdown(level);
   Singleton< Switch >::Instance()->Shutdown(level);
}

//------------------------------------------------------------------------------

fn_name MbModule_Startup = "MbModule.Startup";

void MbModule::Startup(RestartLevel level)
{
   Debug::ft(MbModule_Startup);

   Singleton< Switch >::Instance()->Startup(level);
   Singleton< ToneRegistry >::Instance()->Startup(level);
   Singleton< ToneSilent >::Instance()->Startup(level);
   Singleton< ToneDial >::Instance()->Startup(level);
   Singleton< ToneStutteredDial >::Instance()->Startup(level);
   Singleton< ToneConfirmation >::Instance()->Startup(level);
   Singleton< ToneRingback >::Instance()->Startup(level);
   Singleton< ToneBusy >::Instance()->Startup(level);
   Singleton< ToneCallWaiting >::Instance()->Startup(level);
   Singleton< ToneReorder >::Instance()->Startup(level);
   Singleton< ToneReceiverOffHook >::Instance()->Startup(level);
   Singleton< ToneHeld >::Instance()->Startup(level);
   Singleton< MediaEndptPool >::Instance()->Startup(level);

   //  Define symbols.
   //
   auto reg = Singleton< SymbolRegistry >::Instance();
   reg->BindSymbol("port.silence", Tone::Silence);
   reg->BindSymbol("port.dial", Tone::Dial);
   reg->BindSymbol("port.stutter", Tone::StutteredDial);
   reg->BindSymbol("port.conf", Tone::Confirmation);
   reg->BindSymbol("port.ringback", Tone::Ringback);
   reg->BindSymbol("port.busy", Tone::Busy);
   reg->BindSymbol("port.cwt", Tone::CallWaiting);
   reg->BindSymbol("port.reorder", Tone::Reorder);
   reg->BindSymbol("port.roh", Tone::ReceiverOffHook);
   reg->BindSymbol("port.held", Tone::Held);
}
}
