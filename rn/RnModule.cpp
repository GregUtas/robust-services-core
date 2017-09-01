//==============================================================================
//
//  RnModule.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "RnModule.h"
#include "CbModule.h"
#include "Debug.h"
#include "NbAppIds.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace CallBase;

//------------------------------------------------------------------------------

namespace RoutingNode
{
bool RnModule::Registered = Register();

//------------------------------------------------------------------------------

fn_name RnModule_ctor = "RnModule.ctor";

RnModule::RnModule() : Module(RnModuleId)
{
   Debug::ft(RnModule_ctor);
}

//------------------------------------------------------------------------------

fn_name RnModule_dtor = "RnModule.dtor";

RnModule::~RnModule()
{
   Debug::ft(RnModule_dtor);
}

//------------------------------------------------------------------------------

fn_name RnModule_Register = "RnModule.Register";

bool RnModule::Register()
{
   Debug::ft(RnModule_Register);

   //  Create the modules required by RoutingNode.
   //
   Singleton< CbModule >::Instance();
   Singleton< RnModule >::Instance();
   return true;
}

//------------------------------------------------------------------------------

fn_name RnModule_Shutdown = "RnModule.Shutdown";

void RnModule::Shutdown(RestartLevel level)
{
   Debug::ft(RnModule_Shutdown);
}

//------------------------------------------------------------------------------

fn_name RnModule_Startup = "RnModule.Startup";

void RnModule::Startup(RestartLevel level)
{
   Debug::ft(RnModule_Startup);
}
}