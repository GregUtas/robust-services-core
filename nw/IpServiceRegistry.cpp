//==============================================================================
//
//  IpServiceRegistry.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "IpServiceRegistry.h"
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "IpService.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name IpServiceRegistry_ctor = "IpServiceRegistry.ctor";

IpServiceRegistry::IpServiceRegistry()
{
   Debug::ft(IpServiceRegistry_ctor);

   services_.Init(IpService::MaxId + 1, IpService::CellDiff(), MemProt);
}

//------------------------------------------------------------------------------

fn_name IpServiceRegistry_dtor = "IpServiceRegistry.dtor";

IpServiceRegistry::~IpServiceRegistry()
{
   Debug::ft(IpServiceRegistry_dtor);
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
   Protected::Display(stream, prefix, options);

   stream << prefix << "services [IpServiceId]" << CRLF;
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
   Protected::Patch(selector, arguments);
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
   Debug::ft(IpServiceRegistry_UnbindService);

   services_.Erase(service);
}
}
