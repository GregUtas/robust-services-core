//==============================================================================
//
//  MbModule.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "MbModule.h"
#include "Debug.h"
#include "MbPools.h"
#include "NbAppIds.h"
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
bool MbModule::Registered = Register();

//------------------------------------------------------------------------------

fn_name MbModule_ctor = "MbModule.ctor";

MbModule::MbModule() : Module(MbModuleId)
{
   Debug::ft(MbModule_ctor);
}

//------------------------------------------------------------------------------

fn_name MbModule_dtor = "MbModule.dtor";

MbModule::~MbModule()
{
   Debug::ft(MbModule_dtor);
}

//------------------------------------------------------------------------------

fn_name MbModule_Register = "MbModule.Register";

bool MbModule::Register()
{
   Debug::ft(MbModule_Register);

   //  Create the modules required by MediaBase.
   //
   Singleton< SbModule >::Instance();
   Singleton< MbModule >::Instance();
   return true;
}

//------------------------------------------------------------------------------

fn_name MbModule_Shutdown = "MbModule.Shutdown";

void MbModule::Shutdown(RestartLevel level)
{
   Debug::ft(MbModule_Shutdown);

   Singleton< MediaEndptPool >::Instance()->Shutdown(level);
   Singleton< ToneSilent >::Instance()->Shutdown(level);
   Singleton< ToneDial >::Instance()->Shutdown(level);
   Singleton< ToneStutteredDial >::Instance()->Shutdown(level);
   Singleton< ToneConfirmation >::Instance()->Shutdown(level);
   Singleton< ToneRingback >::Instance()->Shutdown(level);
   Singleton< ToneBusy >::Instance()->Shutdown(level);
   Singleton< ToneCallWaiting >::Instance()->Shutdown(level);
   Singleton< ToneReorder >::Instance()->Shutdown(level);
   Singleton< ToneReceiverOffHook >::Instance()->Shutdown(level);
   Singleton< ToneHeld >::Instance()->Shutdown(level);
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
   if(level < RestartCold) return;

   auto reg = Singleton< SymbolRegistry >::Instance();

   reg->BindSymbol("pool.meps", MediaEndptObjPoolId);

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