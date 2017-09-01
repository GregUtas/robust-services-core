//==============================================================================
//
//  IpPortCfgParm.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "IpPortCfgParm.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "IpPortRegistry.h"
#include "IpService.h"
#include "NwTypes.h"
#include "Singleton.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NodeBase
{
fn_name IpPortCfgParm_ctor = "IpPortCfgParm.ctor";

IpPortCfgParm::IpPortCfgParm(const char* key, const char* def,
   word* field, const char* expl, const IpService* service) :
   CfgIntParm(key, def, field, FirstAppIpPort, LastAppIpPort, expl),
   service_(service)
{
   Debug::ft(IpPortCfgParm_ctor);
}

//------------------------------------------------------------------------------

fn_name IpPortCfgParm_dtor = "IpPortCfgParm.dtor";

IpPortCfgParm::~IpPortCfgParm()
{
   Debug::ft(IpPortCfgParm_dtor);
}

//------------------------------------------------------------------------------

void IpPortCfgParm::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   CfgIntParm::Display(stream, prefix, options);

   stream << prefix << "service : " << strObj(service_) << CRLF;
}

//------------------------------------------------------------------------------

void IpPortCfgParm::Patch(sel_t selector, void* arguments)
{
   CfgParm::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name IpPortCfgParm_SetNextValue = "IpPortCfgParm.SetNextValue";

bool IpPortCfgParm::SetNextValue(word value)
{
   Debug::ft(IpPortCfgParm_SetNextValue);

   //  Check that the port is valid and available for the intended protocol.
   //
   if((value >= FirstAppIpPort) && (value <= LastAppIpPort))
   {
      auto reg = Singleton< IpPortRegistry >::Instance();

      if(reg->GetPort(value, service_->Protocol()) == nullptr)
      {
         return CfgIntParm::SetNextValue(value);
      }
   }

   return false;
}
}
