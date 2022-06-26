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

#include <iosfwd>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <system_error>
#include "Debug.h"
#include "Formatters.h"
#include "MainArgs.h"
#include "RootThread.h"
#include "SignalException.h"
#include "Singleton.h"
#include "SysConsole.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------
//
//  CreateModules() determines what gets included in the build.  Each module
//  resides in its own static library, and all the files that belong to that
//  library reside in a folder with the same name.  The order of modules, from
//  the lowest to the highest layer, is
//                                              dependencies
//  namespace       module      library  nb nt ct nw sb st mb cb pb cn
//  ---------       ------      -------  -----------------------------
//  NodeBase        NbModule    nb
//  NodeTools       NtModule    nt       **
//  NetworkBase     NwModule    nw       **
//  CodeTools       CtModule    ct       ** **
//  SessionBase     SbModule    sb       **       **
//  SessionTools    StModule    st       ** **    ** **
//  MediaBase       MbModule    mb       **       ** **
//  CallBase        CbModule    cb       ** **    ** ** ** **
//  PotsBase        PbModule    pb       ** **    ** ** ** ** **
//  ControlNode     CnModule    cn       **       ** **
//  RoutingNode     RnModule    rn       **       ** **    ** **
//  AccessNode      AnModule    an       **       ** **    ** ** **
//  ServiceNode     SnModule    sn       **       ** **    ** ** **
//  OperationsNode  OnModule    on       **       ** **    ** ** ** **
//  Diplomacy       DipModule   dip      **       **
//  none            main.cpp    none     the desired subset of the above
//
//  RootThread is defined in NodeBase, so a using directive for NodeBase must
//  be included here.  To build only NodeBase, create NbModule.  To include
//  additional layers, add a using directive for the namespace, and create
//  only the module, for the uppermost layer (leaf library) that is required
//  in the build.  That module's constructor will, in turn, create the modules
//  that it requires, and so on transitively.
//
#include "AnModule.h"
//& #include "CbModule.h"
#include "CnModule.h"
#include "CtModule.h"
//& #include "DipModule.h"
//& #include "MbModule.h"
//& #include "NbModule.h"
//& #include "NwModule.h"
//& #include "NtModule.h"
#include "OnModule.h"
//& #include "PbModule.h"
#include "RnModule.h"
//& #include "SbModule.h"
#include "SnModule.h"
//& #include "StModule.h"

using namespace NodeBase;
//& using namespace NodeTools;
using namespace CodeTools;
//& using namespace NetworkBase;
//& using namespace SessionBase;
//& using namespace MediaBase;
//& using namespace CallBase;
//& using namespace SessionTools;
//& using namespace PotsBase;
using namespace OperationsNode;
using namespace ControlNode;
using namespace RoutingNode;
using namespace ServiceNode;
using namespace AccessNode;
//& using namespace Diplomacy;

//------------------------------------------------------------------------------

static void CreateModules()
{
   Debug::ft("CreateModules");

   //& Singleton<NbModule>::Instance();
   //& Singleton<NtModule>::Instance();
   Singleton<CtModule>::Instance();
   //& Singleton<NwModule>::Instance();
   //& Singleton<SbModule>::Instance();
   //& Singleton<StModule>::Instance();
   //& Singleton<MbModule>::Instance();
   //& Singleton<CbModule>::Instance();
   //& Singleton<PbModule>::Instance();
   Singleton<OnModule>::Instance();
   Singleton<CnModule>::Instance();
   Singleton<RnModule>::Instance();
   Singleton<SnModule>::Instance();
   Singleton<AnModule>::Instance();
   //& Singleton<DipModule>::Instance();
}

//------------------------------------------------------------------------------

static main_t LogTrap(const Exception* ex,
   const std::exception* e, int code, const std::ostringstream* stack)
{
   auto& outdev = SysConsole::Out();

   outdev << CRLF << "FATAL EXCEPTION" << CRLF;

   if(e != nullptr)
   {
      outdev << spaces(2) << "type=" << e->what() << CRLF;
      if(code != 0) outdev << spaces(2) << "code=" << code << CRLF;
      if(ex != nullptr) ex->Display(outdev, spaces(2));
      if(stack != nullptr) outdev << stack->str();
   }
   else
   {
      outdev << spaces(2) << "unknown exception" << CRLF;
   }

   outdev << "Enter any string to continue\n";
   outdev << std::flush;

   std::string input;
   auto& indev = SysConsole::In();
   std::getline(indev, input);

   //  The system was dead on arrival, so return 0 to prevent automatic
   //  rebooting.
   //
   return 0;
}

//------------------------------------------------------------------------------

main_t main(int argc, char* argv[])
{
   Debug::ft("main");

   auto& outdev = SysConsole::Out();

   try
   {
      //  Echo and save the arguments.
      //
      outdev << "ROBUST SERVICES CORE" << CRLF;
      outdev << "Entering main(int argc, char* argv[])" << CRLF;
      outdev << "  argc: " << argc << CRLF;

      for(auto i = 0; i < argc; ++i)
      {
         std::string arg(argv[i]);
         MainArgs::PushBack(arg);
         outdev << "  argv[" << i << "]: " << arg << CRLF;
      }

      outdev << std::flush;

      //  Create the desired modules and finish initializing the system.
      //
      CreateModules();
      return RootThread::Main();
   }

   catch(SignalException& sex)
   {
      return LogTrap(&sex, &sex, 0, sex.Stack());
   }

   catch(Exception& ex)
   {
      return LogTrap(&ex, &ex, 0, ex.Stack());
   }

   catch(std::system_error& se)
   {
      return LogTrap(nullptr, &se, se.code().value(), nullptr);
   }

   catch(std::exception& e)
   {
      return LogTrap(nullptr, &e, 0, nullptr);
   }

   catch(...)
   {
      return LogTrap(nullptr, nullptr, 0, nullptr);
   }
}
