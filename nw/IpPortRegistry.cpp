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
#include <sstream>
#include <string>
#include <vector>
#include "Alarm.h"
#include "AlarmRegistry.h"
#include "Algorithms.h"
#include "CfgParmRegistry.h"
#include "Debug.h"
#include "Formatters.h"
#include "FunctionGuard.h"
#include "IpPort.h"
#include "IpService.h"
#include "LocalAddrTest.h"
#include "Log.h"
#include "NwCliParms.h"
#include "NwLogs.h"
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
class LocalAddrCfg : public CfgStrParm
{
public:
   LocalAddrCfg();
   ~LocalAddrCfg();
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

LocalAddrCfg::LocalAddrCfg() : CfgStrParm("ElementIpAddr",
   "127.0.0.1", "element's IP address (check firewall/VPN/etc if routable)")
{
   Debug::ft("LocalAddrCfg.ctor");
}

//------------------------------------------------------------------------------

LocalAddrCfg::~LocalAddrCfg()
{
   Debug::ftnt("LocalAddrCfg.dtor");
}

//------------------------------------------------------------------------------

void LocalAddrCfg::SetCurr()
{
   Debug::ft("LocalAddrCfg.SetCurr");

   FunctionGuard guard(Guard_MemUnprotect);
   CfgStrParm::SetCurr();
   addr_ = SysIpL2Addr(GetCurr());
}

//------------------------------------------------------------------------------

bool LocalAddrCfg::SetNext(c_string input)
{
   Debug::ft("LocalAddrCfg.SetNext");

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

IpPortRegistry::IpPortRegistry() :
   ipv6Enabled_(false),
   localState_(Unverified)
{
   Debug::ft("IpPortRegistry.ctor");

   portq_.Init(IpPort::LinkDiff());
   localAddrCfg_.reset(new LocalAddrCfg);
   Singleton< CfgParmRegistry >::Instance()->BindParm(*localAddrCfg_);
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
      if(!dest.L2AddrMatches(LocalAddr())) return false;
   }

   auto port = dest.GetPort();

   return ((port == NilIpPort) || (GetPort(dest.GetPort()) != nullptr));
}

//------------------------------------------------------------------------------

void IpPortRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Protected::Display(stream, prefix, options);

   stream << prefix << "UseIPv6      : " << UseIPv6() << CRLF;
   stream << prefix << "localAddr    : " << localAddr_.to_str() << CRLF;
   stream << prefix << "localState   : " << localState_ << CRLF;
   stream << prefix << "localAddrCfg : " << strObj(localAddrCfg_.get()) << CRLF;
   stream << prefix << "statsGroup   : " << strObj(statsGroup_.get()) << CRLF;
   stream << prefix << "portq : " << CRLF;
   portq_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

void IpPortRegistry::DisplayLocalAddr(ostream& stream) const
{
   stream << LocalAddr().to_str() << " (" << localState_ << ')';
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

const SysIpL2Addr& IpPortRegistry::LocalAddr()
{
   Debug::ft("IpPortRegistry.LocalAddr");

   //  If this is invoked before we've even been constructed, return
   //  the IPv4 loopback address.
   //
   auto reg = Singleton< IpPortRegistry >::Extant();
   if(reg == nullptr) return SysIpL2Addr::LoopbackIpAddr();
   return reg->localAddr_;
}

//------------------------------------------------------------------------------

void IpPortRegistry::Patch(sel_t selector, void* arguments)
{
   Protected::Patch(selector, arguments);
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
      auto localAddrs = SysIpL3Addr::LocalAddrs();

      for(size_t i = 0; i < localAddrs.size(); ++i)
      {
         if(localAddrs[i].Family() == IPv4) return;
      }
   }

   ipv6Enabled_ = true;
}

//------------------------------------------------------------------------------

void IpPortRegistry::SetLocalAddr()
{
   Debug::ft("IpPortRegistry.SetLocalAddr");

   localAddr_.Nullify();

   //  If the configured address is a loopback address, use it.
   //
   if(localAddrCfg_->Address().IsLoopbackIpAddr())
   {
      localAddr_ = SysIpL2Addr::LoopbackIpAddr();
      return;
   }

   //  If the configured address is a known local address, use it as long as
   //  it's not IPv6 when we're only supposed to use IPv4.  If the configured
   //  address isn't chosen, the platform's IP stack should have arranged the
   //  addresses in order of preference, so use the first acceptable one.  If
   //  no address is acceptable, use the configured address.
   //
   auto localAddrs = SysIpL2Addr::LocalAddrs();

   if(ipv6Enabled_ || (localAddrCfg_->Address().Family() == IPv4))
   {
      for(auto a = localAddrs.cbegin(); a != localAddrs.cend(); ++a)
      {
         if(*a == localAddrCfg_->Address())
         {
            localAddr_ = localAddrCfg_->Address();
            return;
         }
      }
   }

   for(auto a = localAddrs.cbegin(); a != localAddrs.cend(); ++a)
   {
      if(ipv6Enabled_ || (a->Family() == IPv4))
      {
         localAddr_ = *a;
         return;
      }
   }

   localAddr_ = SysIpL2Addr::LoopbackIpAddr();
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
      SetLocalAddr();
      if(statsGroup_ == nullptr) statsGroup_.reset(new IpPortStatsGroup);
      guard.Release();
   }

   for(auto p = portq_.First(); p != nullptr; portq_.Next(p))
   {
      p->Startup(level);
   }
}

//------------------------------------------------------------------------------

fn_name IpPortRegistry_TestAdvance = "IpPortRegistry.TestAdvance";

void IpPortRegistry::TestAdvance()
{
   Debug::ft(IpPortRegistry_TestAdvance);

   FunctionGuard guard(Guard_MemUnprotect);

   switch(localState_)
   {
   case BindFailed:
      localState_ = SendFailed;
      break;
   case SendFailed:
      localState_ = RecvFailed;
      break;
   case RecvFailed:
      localState_ = Verified;
      break;
   default:
      Debug::SwLog(IpPortRegistry_TestAdvance, "invalid state", localState_);
   }
}

//------------------------------------------------------------------------------

void IpPortRegistry::TestBegin()
{
   Debug::ft("IpPortRegistry.TestBegin");

   FunctionGuard guard(Guard_MemUnprotect);
   localState_ = BindFailed;
}

//------------------------------------------------------------------------------

void IpPortRegistry::TestEnd() const
{
   Debug::ft("IpPortRegistry.TestEnd");

   //  Raise an alarm to report the state of the local address.
   //
   auto reg = Singleton< AlarmRegistry >::Instance();
   auto alarm = reg->Find(LocAddrAlarmName);
   auto ok = (localState_ == Verified);

   if(alarm != nullptr)
   {
      auto status = (ok ? NoAlarm : CriticalAlarm);
      auto id = (ok ? NetworkLocalAddrSuccess : NetworkLocalAddrFailure);
      auto log = alarm->Create(NetworkLogGroup, id, status);

      if(log != nullptr)
      {
         if(!ok) *log << Log::Tab << "errval=" << localState_;
         Log::Submit(log);
      }
   }

   //  If the local address test failed, rerun it in 15 seconds.
   //
   if(!ok)
   {
      new LocalAddrRetest(15);
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
