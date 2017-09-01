//==============================================================================
//
//  NwModule.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "NwModule.h"
#include "Debug.h"
#include "IpPortRegistry.h"
#include "IpServiceRegistry.h"
#include "NbAppIds.h"
#include "NbModule.h"
#include "NwIncrement.h"
#include "NwTracer.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysSocket.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
bool NwModule::Registered = Register();

//------------------------------------------------------------------------------

fn_name NwModule_ctor = "NwModule.ctor";

NwModule::NwModule() : Module(NwModuleId)
{
   Debug::ft(NwModule_ctor);
}

//------------------------------------------------------------------------------

fn_name NwModule_dtor = "NwModule.dtor";

NwModule::~NwModule()
{
   Debug::ft(NwModule_dtor);
}

//------------------------------------------------------------------------------

void NwModule::Patch(sel_t selector, void* arguments)
{
   Module::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name NwModule_Register = "NwModule.Register";

bool NwModule::Register()
{
   Debug::ft(NwModule_Register);

   //  Create the modules required by NodeBase.
   //
   Singleton< NbModule >::Instance();
   return true;
}

//------------------------------------------------------------------------------

fn_name NwModule_Shutdown = "NwModule.Shutdown";

void NwModule::Shutdown(RestartLevel level)
{
   Debug::ft(NwModule_Shutdown);

   Singleton< NwIncrement >::Instance()->Shutdown(level);
   Singleton< IpPortRegistry >::Instance()->Shutdown(level);
   Singleton< IpServiceRegistry >::Instance()->Shutdown(level);
   Singleton< NwTracer >::Instance()->Shutdown(level);

   if(level >= RestartCold) SysSocket::StopLayer();
}

//------------------------------------------------------------------------------

fn_name NwModule_Startup = "NwModule.Startup";

void NwModule::Startup(RestartLevel level)
{
   Debug::ft(NwModule_Startup);

   if((level >= RestartCold) && !SysSocket::StartLayer())
   {
      Restart::Initiate(SocketLayerUnavailable, 0);
   }

   Singleton< NwTracer >::Instance()->Startup(level);
   Singleton< IpServiceRegistry >::Instance()->Startup(level);
   Singleton< IpPortRegistry >::Instance()->Startup(level);
   Singleton< NwIncrement >::Instance()->Startup(level);
}
}