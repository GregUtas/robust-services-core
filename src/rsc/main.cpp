//==============================================================================
//
//  main.cpp
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
//------------------------------------------------------------------------------

#include <ostream>
#include <system_error>
#include "Debug.h"
#include "Log.h"
#include "MainArgs.h"
#include "RootThread.h"
#include "SignalException.h"
#include "Singleton.h"
#include "SysConsole.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------
//
//  CreateModules() creates the module for each application that can be enabled,
//  and each of those modules also creates the modules on which it depends.
//    Each module resides in its own static library, and all the files that
//  belong to that library reside in a folder with the same name.  The following
//  table summarizes module dependencies (@ = an application)
//
//                                                dependencies
//  namespace       module      library  nb nt ct nw sb st mb cb pb cn
//  ---------       ------      -------  -----------------------------
//  NodeBase        NbModule      nb
//  NodeTools       NtModule      nt     **
//  NetworkBase     NwModule      nw     **
//  CodeTools       CtModule    @ ct     ** **
//  SessionBase     SbModule      sb     **       **
//  SessionTools    StModule      st     ** **    ** **
//  MediaBase       MbModule      mb     **       ** **
//  CallBase        CbModule      cb     ** **    ** ** ** **
//  PotsBase        PbModule      pb     ** **    ** ** ** ** **
//  ControlNode     CnModule    @ cn     **       ** **
//  RoutingNode     RnModule    @ rn     **       ** **    ** **
//  AccessNode      AnModule    @ an     **       ** **    ** ** **
//  ServiceNode     SnModule    @ sn     **       ** **    ** ** **
//  OperationsNode  OnModule    @ on     **       ** **    ** ** ** **
//  Diplomacy       DipModule   @ dip    **       **
//  none            main.cpp      none   the desired subset of applications
//
#include "AnModule.h"
#include "CnModule.h"
#include "CtModule.h"
#include "DipModule.h"
#include "OnModule.h"
#include "RnModule.h"
#include "SnModule.h"

using namespace NodeBase;
using namespace CodeTools;
using namespace OperationsNode;
using namespace ControlNode;
using namespace RoutingNode;
using namespace ServiceNode;
using namespace AccessNode;
using namespace Diplomacy;

//------------------------------------------------------------------------------

static void CreateModules()
{
   Debug::ft("CreateModules");

   Singleton<CtModule>::Instance();
   Singleton<OnModule>::Instance();
   Singleton<CnModule>::Instance();
   Singleton<RnModule>::Instance();
   Singleton<SnModule>::Instance();
   Singleton<AnModule>::Instance();
   Singleton<DipModule>::Instance();
}

//------------------------------------------------------------------------------

main_t main(int argc, char* argv[])
{
   Debug::ft("main");

   auto& outdev = SysConsole::Out();

   try
   {
      //  Echo and save the arguments.  Create the desired
      //  modules and finish initializing the system.
      //
      outdev << "ROBUST SERVICES CORE" << CRLF;
      MainArgs::EchoAndSaveArgs(argc, argv);
      CreateModules();
      return RootThread::Main();
   }

   catch(SignalException& sex)
   {
      return Log::TrapInMain(&sex, &sex, 0, sex.Stack());
   }

   catch(Exception& ex)
   {
      return Log::TrapInMain(&ex, &ex, 0, ex.Stack());
   }

   catch(std::system_error& se)
   {
      return Log::TrapInMain(nullptr, &se, se.code().value(), nullptr);
   }

   catch(std::exception& e)
   {
      return Log::TrapInMain(nullptr, &e, 0, nullptr);
   }

   catch(...)
   {
      return Log::TrapInMain(nullptr, nullptr, 0, nullptr);
   }
}
