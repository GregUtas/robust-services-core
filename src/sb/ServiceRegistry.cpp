//==============================================================================
//
//  ServiceRegistry.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "ServiceRegistry.h"
#include <iomanip>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "Service.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
ServiceRegistry::ServiceRegistry()
{
   Debug::ft("ServiceRegistry.ctor");

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

bool ServiceRegistry::BindService(Service& service)
{
   Debug::ft("ServiceRegistry.BindService");

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

void ServiceRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fixed_string ServiceHeader =
   " Id  Enabled  Modifier  States  Handlers  Triggers  Name";
// |  3        9        10       8        10        10..<name>

size_t ServiceRegistry::Summarize(ostream& stream, uint32_t selector) const
{
   stream << ServiceHeader << CRLF;

   for(auto s = services_.First(); s != nullptr; services_.Next(s))
   {
      stream << setw(3) << s->Sid();
      stream << setw(9) << (s->GetStatus() == Service::Enabled);
      stream << setw(10) << s->IsModifier();
      stream << setw(8) << s->States().Size();
      stream << setw(10) << s->Handlers().Size();
      stream << setw(10) << s->Triggers().Size();
      stream << spaces(2) << strClass(s) << CRLF;
   }

   return services_.Size();
}

//------------------------------------------------------------------------------

void ServiceRegistry::UnbindService(Service& service)
{
   Debug::ftnt("ServiceRegistry.UnbindService");

   services_.Erase(service);
}
}
