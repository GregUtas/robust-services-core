//==============================================================================
//
//  CnModule.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CnModule.h"
#include "Debug.h"
#include "NbAppIds.h"
#include "SbModule.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace SessionBase;

//------------------------------------------------------------------------------

namespace ControlNode
{
bool CnModule::Registered = Register();

//------------------------------------------------------------------------------

fn_name CnModule_ctor = "CnModule.ctor";

CnModule::CnModule() : Module(CnModuleId)
{
   Debug::ft(CnModule_ctor);
}

//------------------------------------------------------------------------------

fn_name CnModule_dtor = "CnModule.dtor";

CnModule::~CnModule()
{
   Debug::ft(CnModule_dtor);
}

//------------------------------------------------------------------------------

fn_name CnModule_Register = "CnModule.Register";

bool CnModule::Register()
{
   Debug::ft(CnModule_Register);

   //  Include the modules required by ControlNode.
   //
   Singleton< SbModule >::Instance();
   Singleton< CnModule >::Instance();
   return true;
}

//------------------------------------------------------------------------------

fn_name CnModule_Shutdown = "CnModule.Shutdown";

void CnModule::Shutdown(RestartLevel level)
{
   Debug::ft(CnModule_Shutdown);
}

//------------------------------------------------------------------------------

fn_name CnModule_Startup = "CnModule.Startup";

void CnModule::Startup(RestartLevel level)
{
   Debug::ft(CnModule_Startup);
}
}