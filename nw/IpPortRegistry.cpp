//==============================================================================
//
//  IpPortRegistry.cpp
//
//  Copyright (C) 2013-2021  Greg Utas
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
#include "IpPortRegistry.h"
#include "CfgStrParm.h"
#include "StatisticsGroup.h"
#include <cstddef>
#include <ostream>
#include <string>
#include <vector>
#include "Algorithms.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "IpPort.h"
#include "IpService.h"
#include "NwCliParms.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Configuration parameter for this element's IP address.
//
class HostAddrCfg : public CfgStrParm
{
public:
   HostAddrCfg();
   ~HostAddrCfg();
   const SysIpL2Addr& Address() const { return addr_; }
protected:
   void SetCurr() override;
private:
   RestartLevel RestartRequired() const override { return RestartCold; }
   bool SetNext(c_string input) override;

   //  Kept in synch with the string version of the element's address.
   //
   SysIpL2Addr addr_;
};

//------------------------------------------------------------------------------

HostAddrCfg::HostAddrCfg() : CfgStrParm("ElementIpAddr",
   "127.0.0.1", "element's IP address")
{
   Debug::ft("HostAddrCfg.ctor");
}

//------------------------------------------------------------------------------

HostAddrCfg::~HostAddrCfg()
{
   Debug::ftnt("HostAddrCfg.dtor");
}

//------------------------------------------------------------------------------

void HostAddrCfg::SetCurr()
{
   Debug::ft("HostAddrCfg.SetCurr");

   FunctionGuard guard(Guard_MemUnprotect);
   CfgStrParm::SetCurr();
   addr_ = SysIpL2Addr(GetCurr());
}

//------------------------------------------------------------------------------

bool HostAddrCfg::SetNext(c_string input)
{
   Debug::ft("HostAddrCfg.SetNext");

   SysIpL2Addr addr(input);
   if(!addr.IsValid()) return false;
   return CfgStrParm::SetNext(input);
}

//==============================================================================

class IpPortStatsGroup : public StatisticsGroup
{
public:
   IpPortStatsGroup();
   ~IpPortStatsGroup();
   void DisplayStats
      (ostream& stream, id_t id, const Flags& options) const override;
};

//------------------------------------------------------------------------------

IpPortStatsGroup::IpPortStatsGroup() : StatisticsGroup("IpPorts [ipport_t]")
{
   Debug::ft("IpPortStatsGroup.ctor");
}

//------------------------------------------------------------------------------

IpPortStatsGroup::~IpPortStatsGroup()
{
   Debug::ftnt("IpPortStatsGroup.dtor");
}

//------------------------------------------------------------------------------

void IpPortStatsGroup::DisplayStats
   (ostream& stream, id_t id, const Flags& options) const
{
   Debug::ft("IpPortStatsGroup.DisplayStats");

   StatisticsGroup::DisplayStats(stream, id, options);

   auto reg = Singleton< IpPortRegistry >::Instance();

   if(id == 0)
   {
      auto& ports = reg->Ports();

      for(auto p = ports.First(); p != nullptr; ports.Next(p))
      {
         p->DisplayStats(stream, options);
      }
   }
   else
   {
      auto p = reg->GetPort(id);

      if(p == nullptr)
      {
         stream << spaces(2) << NoIpPortExpl << CRLF;
         return;
      }

      p->DisplayStats(stream, options);
   }
}

//==============================================================================

IpPortRegistry::IpPortRegistry() : ipv6Enabled_(false)
{
   Debug::ft("IpPortRegistry.ctor");

   portq_.Init(IpPort::LinkDiff());
   hostAddrCfg_.reset(new HostAddrCfg);
   Singleton< CfgParmRegistry >::Instance()->BindParm(*hostAddrCfg_);
   statsGroup_.reset(new IpPortStatsGroup);
}

//------------------------------------------------------------------------------

fn_name IpPortRegistry_dtor = "IpPortRegistry.dtor";

IpPortRegistry::~IpPortRegistry()
{
   Debug::ftnt(IpPortRegistry_dtor);

   Debug::SwLog(IpPortRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

fn_name IpPortRegistry_BindPort = "IpPortRegistry.BindPort";

bool IpPortRegistry::BindPort(IpPort& port)
{
   Debug::ft(IpPortRegistry_BindPort);

   //  Sort entries by port number.  Generate a log and reject overbinding.
   //
   auto pid = port.GetPort();

   IpPort* prev = nullptr;
   IpPort* curr = portq_.First();

   //  Skip lower-numbered ports.
   //
   while(curr != nullptr)
   {
      if(curr->GetPort() >= pid) break;
      prev = curr;
      portq_.Next(curr);
   }

   auto newpro = port.GetService()->Protocol();

   while(curr != nullptr)
   {
      if(curr->GetPort() > pid) break;

      //  CURR is already using this port number.  This is only allowed if
      //  CURR supports UDP or TCP and the new port supports the other.
      //
      auto oldpro = curr->GetService()->Protocol();

      if((newpro == IpAny) || (oldpro == IpAny) || (newpro == oldpro))
      {
         Debug::SwLog(IpPortRegistry_BindPort,
            "port already in use", pack3(newpro, oldpro, pid));
         return false;
      }

      prev = curr;
      portq_.Next(curr);
   }

   portq_.Insert(prev, port);
   return true;
}

//------------------------------------------------------------------------------

bool IpPortRegistry::CanBypassStack
   (const SysIpL3Addr& srce, const SysIpL3Addr& dest) const
{
   Debug::ft("IpPortRegistry.CanBypassStack");

   if(!srce.L2AddrMatches(dest) && !dest.IsLoopbackIpAddr())
   {
      if(!dest.L2AddrMatches(HostAddress())) return false;
   }

   auto port = dest.GetPort();

   return ((port == NilIpPort) || (GetPort(dest.GetPort()) != nullptr));
}

//------------------------------------------------------------------------------

void IpPortRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "UseIPv6     : " << UseIPv6() << CRLF;
   stream << prefix << "HostAddr    : " << hostAddr_.to_str() << CRLF;
   stream << prefix << "hostAddrCfg : " << strObj(hostAddrCfg_.get()) << CRLF;
   stream << prefix << "statsGroup  : " << strObj(statsGroup_.get()) << CRLF;
   stream << prefix << "portq : " << CRLF;
   portq_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

IpPort* IpPortRegistry::GetPort(ipport_t port, IpProtocol protocol) const
{
   for(auto p = portq_.First(); p != nullptr; portq_.Next(p))
   {
      if(p->GetPort() == port)
      {
         if(protocol == IpAny) return p;
         if(p->GetService()->Protocol() == protocol) return p;
      }
   }

   return nullptr;
}

//------------------------------------------------------------------------------

const SysIpL2Addr& IpPortRegistry::HostAddress()
{
   Debug::ft("IpPortRegistry.HostAddress");

   //  If this is invoked before we've even been constructed, return
   //  the IPv4 loopback address.
   //
   auto reg = Singleton< IpPortRegistry >::Extant();
   if(reg == nullptr) return SysIpL2Addr::LoopbackIpAddr();
   return reg->hostAddr_;
}

//------------------------------------------------------------------------------

void IpPortRegistry::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

void IpPortRegistry::SetHostAddress()
{
   Debug::ft("IpPortRegistry.SetHostAddress");

   hostAddr_.Nullify();

   //  If the configured address is a loopback address, use it.
   //
   if(hostAddrCfg_->Address().IsLoopbackIpAddr())
   {
      hostAddr_ = SysIpL2Addr::LoopbackIpAddr();
      return;
   }

   //  Get this element's addresses.  If the configured address is on the list,
   //  use it as long as it's not IPv6 when we're only supposed to use IPv4.
   //  If the configured address isn't chosen, the platform's IP stack should
   //  have arranged the addresses in order of preference, so use the first one
   //  that is acceptable.  If the list is empty, use the configured address.
   //
   auto hostaddrs = SysIpL3Addr::HostAddresses();

   if(ipv6Enabled_ || (hostAddrCfg_->Address().Family() == IPv4))
   {
      for(size_t i = 0; i < hostaddrs.size(); ++i)
      {
         if(hostaddrs[i].L2AddrMatches(hostAddrCfg_->Address()))
         {
            hostAddr_ = hostAddrCfg_->Address();
            return;
         }
      }
   }

   for(size_t i = 0; i < hostaddrs.size(); ++i)
   {
      if(ipv6Enabled_ || (hostaddrs[i].Family() == IPv4))
      {
         hostAddr_ = hostaddrs[i];
         return;
      }
   }

   if(!hostaddrs.empty()) hostAddr_ = hostaddrs[0];
}

//------------------------------------------------------------------------------

void IpPortRegistry::SetIPv6()
{
   Debug::ft("IpPortRegistry.SetIPv6");

   ipv6Enabled_ = SysIpL2Addr::SupportsIPv6();

   //  If this element only has IPv6 addresses, IPv6 must be enabled.
   //
   if(!ipv6Enabled_)
   {
      auto hostaddrs = SysIpL3Addr::HostAddresses();

      for(size_t i = 0; i < hostaddrs.size(); ++i)
      {
         if(hostaddrs[i].Family() == IPv4) return;
      }
   }

   ipv6Enabled_ = true;
}

//------------------------------------------------------------------------------

void IpPortRegistry::Shutdown(RestartLevel level)
{
   Debug::ft("IpPortRegistry.Shutdown");

   for(auto p = portq_.First(); p != nullptr; portq_.Next(p))
   {
      p->Shutdown(level);
   }

   FunctionGuard guard(Guard_MemUnprotect);
   Restart::Release(statsGroup_);
}

//------------------------------------------------------------------------------

void IpPortRegistry::Startup(RestartLevel level)
{
   Debug::ft("IpPortRegistry.Startup");

   if(level >= RestartCold)
   {
      FunctionGuard guard(Guard_MemUnprotect);
      SetIPv6();
      SetHostAddress();
      if(statsGroup_ == nullptr) statsGroup_.reset(new IpPortStatsGroup);
      guard.Release();
   }

   for(auto p = portq_.First(); p != nullptr; portq_.Next(p))
   {
      p->Startup(level);
   }
}

//------------------------------------------------------------------------------

void IpPortRegistry::UnbindPort(IpPort& port)
{
   Debug::ftnt("IpPortRegistry.UnbindPort");

   portq_.Exq(port);
}

//------------------------------------------------------------------------------

bool IpPortRegistry::UseIPv6()
{
   auto reg = Singleton< IpPortRegistry >::Extant();
   if(reg == nullptr) return SysIpL2Addr::SupportsIPv6();
   return reg->ipv6Enabled_;
}
}
