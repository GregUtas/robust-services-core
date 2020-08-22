//==============================================================================
//
//  IpServiceRegistry.cpp
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
#include "IpServiceRegistry.h"
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "IpService.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fn_name IpServiceRegistry_ctor = "IpServiceRegistry.ctor";

IpServiceRegistry::IpServiceRegistry()
{
   Debug::ft(IpServiceRegistry_ctor);

   services_.Init(IpService::MaxId, IpService::CellDiff(), MemImmutable);
}

//------------------------------------------------------------------------------

fn_name IpServiceRegistry_dtor = "IpServiceRegistry.dtor";

IpServiceRegistry::~IpServiceRegistry()
{
   Debug::ftnt(IpServiceRegistry_dtor);

   Debug::SwLog(IpServiceRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

fn_name IpServiceRegistry_BindService = "IpServiceRegistry.BindService";

bool IpServiceRegistry::BindService(IpService& service)
{
   Debug::ft(IpServiceRegistry_BindService);

   return services_.Insert(service);
}

//------------------------------------------------------------------------------

void IpServiceRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "services [id_t]" << CRLF;
   services_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

IpService* IpServiceRegistry::GetService(const string& name) const
{
   for(auto s = services_.First(); s != nullptr; services_.Next(s))
   {
      if(s->Name() == name) return s;
   }

   return nullptr;
}

//------------------------------------------------------------------------------

void IpServiceRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name IpServiceRegistry_Shutdown = "IpServiceRegistry.Shutdown";

void IpServiceRegistry::Shutdown(RestartLevel level)
{
   Debug::ft(IpServiceRegistry_Shutdown);

   for(auto s = services_.First(); s != nullptr; services_.Next(s))
   {
      s->Shutdown(level);
   }
}

//------------------------------------------------------------------------------

fn_name IpServiceRegistry_Startup = "IpServiceRegistry.Startup";

void IpServiceRegistry::Startup(RestartLevel level)
{
   Debug::ft(IpServiceRegistry_Startup);

   for(auto s = services_.First(); s != nullptr; services_.Next(s))
   {
      s->Startup(level);
   }
}

//------------------------------------------------------------------------------

fn_name IpServiceRegistry_UnbindService = "IpServiceRegistry.UnbindService";

void IpServiceRegistry::UnbindService(IpService& service)
{
   Debug::ftnt(IpServiceRegistry_UnbindService);

   services_.Erase(service);
}
}
