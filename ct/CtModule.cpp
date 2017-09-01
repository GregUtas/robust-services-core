//==============================================================================
//
//  CtModule.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "CtModule.h"
#include "CodeIncrement.h"
#include "CxxExecute.h"
#include "CxxRoot.h"
#include "CxxSymbols.h"
#include "Debug.h"
#include "Library.h"
#include "NbAppIds.h"
#include "NbModule.h"
#include "Singleton.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace CodeTools
{
bool CtModule::Registered = Register();

//------------------------------------------------------------------------------

fn_name CtModule_ctor = "CtModule.ctor";

CtModule::CtModule() : Module(CtModuleId)
{
   Debug::ft(CtModule_ctor);
}

//------------------------------------------------------------------------------

fn_name CtModule_dtor = "CtModule.dtor";

CtModule::~CtModule()
{
   Debug::ft(CtModule_dtor);
}

//------------------------------------------------------------------------------

fn_name CtModule_Register = "CtModule.Register";

bool CtModule::Register()
{
   Debug::ft(CtModule_Register);

   //  Create the modules required by CodeTools.
   //
   Singleton< NbModule >::Instance();
   return true;
}

//------------------------------------------------------------------------------

fn_name CtModule_Shutdown = "CtModule.Shutdown";

void CtModule::Shutdown(RestartLevel level)
{
   Debug::ft(CtModule_Shutdown);

   Context::Shutdown(level);
   Singleton< CxxRoot >::Instance()->Shutdown(level);
   Singleton< Library >::Instance()->Shutdown(level);
   Singleton< CxxSymbols >::Instance()->Shutdown(level);
   Singleton< CodeIncrement >::Instance()->Shutdown(level);
}

//------------------------------------------------------------------------------

fn_name CtModule_Startup = "CtModule.Startup";

void CtModule::Startup(RestartLevel level)
{
   Debug::ft(CtModule_Startup);

   //  Create/start singletons.
   //
   Singleton< CodeIncrement >::Instance()->Startup(level);
   Singleton< CxxSymbols >::Instance()->Startup(level);
   Singleton< Library >::Instance()->Startup(level);
   Singleton< CxxRoot >::Instance()->Startup(level);
   Context::Startup(level);
}
}