//==============================================================================
//
//  OnModule.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "OnModule.h"
#include "CnModule.h"
#include "Debug.h"
#include "NbAppIds.h"
#include "PbModule.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace ControlNode;
using namespace PotsBase;

//------------------------------------------------------------------------------

namespace OperationsNode
{
bool OnModule::Registered = Register();

//------------------------------------------------------------------------------

fn_name OnModule_ctor = "OnModule.ctor";

OnModule::OnModule() : Module(OnModuleId)
{
   Debug::ft(OnModule_ctor);
}

//------------------------------------------------------------------------------

fn_name OnModule_dtor = "OnModule.dtor";

OnModule::~OnModule()
{
   Debug::ft(OnModule_dtor);
}

//------------------------------------------------------------------------------

fn_name OnModule_Register = "OnModule.Register";

bool OnModule::Register()
{
   Debug::ft(OnModule_Register);

   //  Create the modules required by OperationsNode.
   //
   Singleton< CnModule >::Instance();
   Singleton< PbModule >::Instance();
   Singleton< OnModule >::Instance();
   return true;
}

//------------------------------------------------------------------------------

fn_name OnModule_Shutdown = "OnModule.Shutdown";

void OnModule::Shutdown(RestartLevel level)
{
   Debug::ft(OnModule_Shutdown);
}

//------------------------------------------------------------------------------

fn_name OnModule_Startup = "OnModule.Startup";

void OnModule::Startup(RestartLevel level)
{
   Debug::ft(OnModule_Startup);
}
}