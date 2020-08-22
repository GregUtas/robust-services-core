//==============================================================================
//
//  ServiceRegistry.cpp
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
#include "ServiceRegistry.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "Service.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name ServiceRegistry_ctor = "ServiceRegistry.ctor";

ServiceRegistry::ServiceRegistry()
{
   Debug::ft(ServiceRegistry_ctor);

   services_.Init(Service::MaxId, Service::CellDiff(), MemImmutable);
}

//------------------------------------------------------------------------------

fn_name ServiceRegistry_dtor = "ServiceRegistry.dtor";

ServiceRegistry::~ServiceRegistry()
{
   Debug::ftnt(ServiceRegistry_dtor);

   Debug::SwLog(ServiceRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

fn_name ServiceRegistry_BindService = "ServiceRegistry.BindService";

bool ServiceRegistry::BindService(Service& service)
{
   Debug::ft(ServiceRegistry_BindService);

   return services_.Insert(service);
}

//------------------------------------------------------------------------------

void ServiceRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "services [ServiceId]" << CRLF;
   services_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

Service* ServiceRegistry::GetService(ServiceId sid) const
{
   return services_.At(sid);
}

//------------------------------------------------------------------------------

void ServiceRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector,arguments);
}

//------------------------------------------------------------------------------

fn_name ServiceRegistry_UnbindService = "ServiceRegistry.UnbindService";

void ServiceRegistry::UnbindService(Service& service)
{
   Debug::ftnt(ServiceRegistry_UnbindService);

   services_.Erase(service);
}
}
