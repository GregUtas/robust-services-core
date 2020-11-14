//==============================================================================
//
//  ServiceCodeRegistry.cpp
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
#include "ServiceCodeRegistry.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "Restart.h"
#include "SbAppIds.h"
#include "ServiceRegistry.h"
#include "Singleton.h"
#include "SymbolRegistry.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace CallBase
{
ServiceCodeRegistry::ServiceCodeRegistry()
{
   Debug::ft("ServiceCodeRegistry.ctor");

   for(auto i = 0; i <= Address::LastSC; ++i)
   {
      codeToService_[i] = NIL_ID;
   }
}

//------------------------------------------------------------------------------

fn_name ServiceCodeRegistry_dtor = "ServiceCodeRegistry.dtor";

ServiceCodeRegistry::~ServiceCodeRegistry()
{
   Debug::ftnt(ServiceCodeRegistry_dtor);

   Debug::SwLog(ServiceCodeRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

void ServiceCodeRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "codeToService [Address::SC]" << CRLF;

   for(auto i = 0; i <= Address::LastSC; ++i)
   {
      auto lead = prefix + spaces(2);
      auto sid = codeToService_[i];

      if(sid != NIL_ID)
      {
         auto svc = Singleton< ServiceRegistry >::Instance()->GetService(sid);

         stream << lead << strIndex(i);

         if(svc != nullptr)
            stream << strObj(svc) << CRLF;
         else
            stream << sid << " (no service registered)" << CRLF;
      }
   }
}

//------------------------------------------------------------------------------

fn_name ServiceCodeRegistry_GetService = "ServiceCodeRegistry.GetService";

ServiceId ServiceCodeRegistry::GetService(Address::SC sc) const
{
   Debug::ft(ServiceCodeRegistry_GetService);

   if(Address::IsValidSC(sc)) return codeToService_[sc];

   Debug::SwLog(ServiceCodeRegistry_GetService, "invalid service code", sc);
   return NIL_ID;
}

//------------------------------------------------------------------------------

fn_name ServiceCodeRegistry_SetService = "ServiceCodeRegistry.SetService";

void ServiceCodeRegistry::SetService(Address::SC sc, ServiceId sid)
{
   Debug::ft(ServiceCodeRegistry_SetService);

   if(!Address::IsValidSC(sc))
   {
      Debug::SwLog(ServiceCodeRegistry_SetService, "invalid service code", sc);
      return;
   }

   codeToService_[sc] = sid;
}

//------------------------------------------------------------------------------

void ServiceCodeRegistry::Startup(RestartLevel level)
{
   Debug::ft("ServiceCodeRegistry.Startup");

   //  Define service codes if our registry was just created.  These
   //  are fixed but would be configurable in a production system.
   //
   if(Restart::ClearsMemory(MemType()))
   {
      SetService(33, PotsWmlActivation);
      SetService(34, PotsWmlDeactivation);
      SetService(70, PotsCcwServiceId);
      SetService(72, PotsCfuActivation);
      SetService(73, PotsCfuDeactivation);
   }

   //  Define service code symbols.
   //
   auto reg = Singleton< SymbolRegistry >::Instance();
   reg->BindSymbol("sc.wml.activation", "*33");
   reg->BindSymbol("sc.wml.deactivation", "*34");
   reg->BindSymbol("sc.ccw", "*70");
   reg->BindSymbol("sc.cfu.activation", "*72");
   reg->BindSymbol("sc.cfu.deactivation", "*73");
}
}
