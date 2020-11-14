//==============================================================================
//
//  IpPortRegistry.cpp
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
#include "IpPortRegistry.h"
#include "CfgStrParm.h"
#include "StatisticsGroup.h"
#include <ostream>
#include <string>
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
//  Configuration parameter for the host IP address.
//
class HostAddrCfg : public CfgStrParm
{
public:
   HostAddrCfg();
   ~HostAddrCfg();
   SysIpL2Addr Address() const { return addr_; }
protected:
   void SetCurr() override;
private:
   bool SetNext(c_string input) override;

   //  Kept in synch with the string version of the element's address.
   //
   SysIpL2Addr addr_;
};

//------------------------------------------------------------------------------

HostAddrCfg::HostAddrCfg() : CfgStrParm("ElementDefaultAddr",
   "127.0.0.1", "element's default IP address (n.n.n.n)")
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

IpPortRegistry::IpPortRegistry()
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

   if((dest.GetIpV4Addr() != srce.GetIpV4Addr()) &&
      (dest.GetIpV4Addr() != SysIpL2Addr::LoopbackAddr().GetIpV4Addr()))
   {
      auto host = HostAddress();
      if(dest.GetIpV4Addr() != host.GetIpV4Addr()) return false;
   }

   auto port = dest.GetPort();

   return ((port == NilIpPort) || (GetPort(dest.GetPort()) != nullptr));
}

//------------------------------------------------------------------------------

void IpPortRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

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

SysIpL2Addr IpPortRegistry::HostAddress()
{
   Debug::ft("IpPortRegistry.HostAddress");

   auto reg = Singleton< IpPortRegistry >::Extant();
   if(reg == nullptr) return SysIpL2Addr::LoopbackAddr();

   if(reg->hostAddr_.IsValid()) return reg->hostAddr_;

   string name;
   string service;
   IpProtocol proto;

   if(SysIpL2Addr::HostName(name))
   {
      SysIpL3Addr host(name, service, proto);

      FunctionGuard guard(Guard_MemUnprotect);
      reg->hostAddr_ = host;
      return host;
   }

   return reg->hostAddrCfg_->Address();
}

//------------------------------------------------------------------------------

void IpPortRegistry::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
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

   //  The host address is frequently required and may be expensive to
   //  look up.  It is therefore cached during initialization, after
   //  which a restart is required to change it.
   //
   FunctionGuard guard(Guard_MemUnprotect);
   HostAddress();
   if(statsGroup_ == nullptr) statsGroup_.reset(new IpPortStatsGroup);
   guard.Release();

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
}
