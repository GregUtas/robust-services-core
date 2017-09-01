//==============================================================================
//
//  ServiceRegistry.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "ServiceRegistry.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "Service.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name ServiceRegistry_ctor = "ServiceRegistry.ctor";

ServiceRegistry::ServiceRegistry()
{
   Debug::ft(ServiceRegistry_ctor);

   services_.Init(Service::MaxId + 1, Service::CellDiff(), MemProt);
}

//------------------------------------------------------------------------------

fn_name ServiceRegistry_dtor = "ServiceRegistry.dtor";

ServiceRegistry::~ServiceRegistry()
{
   Debug::ft(ServiceRegistry_dtor);
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
   Protected::Display(stream, prefix, options);

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
   Protected::Patch(selector,arguments);
}

//------------------------------------------------------------------------------

fn_name ServiceRegistry_UnbindService = "ServiceRegistry.UnbindService";

void ServiceRegistry::UnbindService(Service& service)
{
   Debug::ft(ServiceRegistry_UnbindService);

   services_.Erase(service);
}
}
