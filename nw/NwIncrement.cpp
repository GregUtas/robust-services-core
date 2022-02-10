//==============================================================================
//
//  NwIncrement.cpp
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
#include "NwIncrement.h"
#include "CliCommand.h"
#include "CliText.h"
#include "CliTextParm.h"
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include "CliBoolParm.h"
#include "CliThread.h"
#include "Debug.h"
#include "Duration.h"
#include "Formatters.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "IpService.h"
#include "LocalAddrTest.h"
#include "NbCliParms.h"
#include "NwCliParms.h"
#include "NwTracer.h"
#include "NwTypes.h"
#include "Q1Way.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"
#include "ThisThread.h"
#include "Tool.h"
#include "ToolTypes.h"

using namespace NodeBase;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  The CLEAR command.
//
NwClearWhatParm::NwClearWhatParm()
{
   BindText(*new PeerText, NwClearCommand::PeerIndex);
   BindText(*new PeersText, NwClearCommand::PeersIndex);
   BindText(*new PortText, NwClearCommand::PortIndex);
   BindText(*new PortsText, NwClearCommand::PortsIndex);
}

NwClearCommand::NwClearCommand(bool bind) : ClearCommand(false)
{
   if(bind) BindParm(*new NwClearWhatParm);
}

void NwClearCommand::Patch(sel_t selector, void* arguments)
{
   ClearCommand::Patch(selector, arguments);
}

word NwClearCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("NwClearCommand.ProcessSubcommand");

   TraceRc rc;
   auto nwt = Singleton< NwTracer >::Instance();
   word id = 0;
   SysIpL3Addr targ;

   switch(index)
   {
   case PeerIndex:
      if(!GetIpL3Addr(targ, *this, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = nwt->SelectPeer(targ, TraceDefault);
      break;
   case PeersIndex:
      if(!cli.EndOfInput()) return -1;
      rc = nwt->ClearSelections(TracePeer);
      break;
   case PortIndex:
      if(!GetIntParm(id, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = nwt->SelectPort(id, TraceDefault);
      break;
   case PortsIndex:
      if(!cli.EndOfInput()) return -1;
      rc = nwt->ClearSelections(TracePort);
      break;
   default:
      return ClearCommand::ProcessSubcommand(cli, index);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The EXCLUDE command.
//
NwExcludeWhatParm::NwExcludeWhatParm()
{
   BindText(*new PeerText, NwExcludeCommand::ExcludePeerIndex);
   BindText(*new PortText, NwExcludeCommand::ExcludePortIndex);
}

NwExcludeCommand::NwExcludeCommand(bool bind) : ExcludeCommand(false)
{
   if(bind) BindParm(*new NwExcludeWhatParm);
}

void NwExcludeCommand::Patch(sel_t selector, void* arguments)
{
   ExcludeCommand::Patch(selector, arguments);
}

word NwExcludeCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("NwExcludeCommand.ProcessSubcommand");

   TraceRc rc;
   auto nwt = Singleton< NwTracer >::Instance();
   word id;
   SysIpL3Addr targ;

   switch(index)
   {
   case ExcludePeerIndex:
      if(!GetIpL3Addr(targ, *this, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = nwt->SelectPeer(targ, TraceExcluded);
      break;
   case ExcludePortIndex:
      if(!GetIntParm(id, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = nwt->SelectPort(id, TraceExcluded);
      break;
   default:
      return ExcludeCommand::ProcessSubcommand(cli, index);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The INCLUDE command.
//
NwIncludeWhatParm::NwIncludeWhatParm()
{
   BindText(*new PeerText, NwIncludeCommand::IncludePeerIndex);
   BindText(*new PortText, NwIncludeCommand::IncludePortIndex);
}

NwIncludeCommand::NwIncludeCommand(bool bind) : IncludeCommand(false)
{
   if(bind) BindParm(*new NwIncludeWhatParm);
}

void NwIncludeCommand::Patch(sel_t selector, void* arguments)
{
   IncludeCommand::Patch(selector, arguments);
}

word NwIncludeCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("NwIncludeCommand.ProcessSubcommand");

   TraceRc rc;
   auto nwt = Singleton< NwTracer >::Instance();
   word id;
   SysIpL3Addr targ;

   switch(index)
   {
   case IncludePeerIndex:
      if(!GetIpL3Addr(targ, *this, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = nwt->SelectPeer(targ, TraceIncluded);
      break;
   case IncludePortIndex:
      if(!GetIntParm(id, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      rc = nwt->SelectPort(id, TraceIncluded);
      break;
   default:
      return IncludeCommand::ProcessSubcommand(cli, index);
   }

   return ExplainTraceRc(cli, rc);
}

//------------------------------------------------------------------------------
//
//  The IP command.
//
class LocalAddrText : public CliText
{
public: LocalAddrText();
};

class NameToAddrText : public CliText
{
public: NameToAddrText();
};

class IpAction : public CliTextParm
{
public: IpAction();
};

class IpCommand : public CliCommand
{
public:
   IpCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string LocalNameTextStr = "localname";
fixed_string LocalNameTextExpl = "displays this element's host name";

fixed_string UsesIPv6TextStr = "usesipv6";
fixed_string UsesIPv6TextExpl = "displays whether this element uses IPv6";

fixed_string LocalAddrTextStr = "localaddr";
fixed_string LocalAddrTextExpl = "displays this element's IP address";

fixed_string LocalAddrExpl = "retest local address? (default=f)";

LocalAddrText::LocalAddrText() : CliText(LocalAddrTextExpl, LocalAddrTextStr)
{
   BindParm(*new CliBoolParm(LocalAddrExpl, true));
}

fixed_string LocalAddrsTextStr = "localaddrs";
fixed_string LocalAddrsTextExpl = "displays this element's IP addresses";

fixed_string NameToAddrTextStr = "nametoaddr";
fixed_string NameToAddrTextExpl =
   "maps a host name/service name to an IP address";

fixed_string AddrToNameTextStr = "addrtoname";
fixed_string AddrToNameTextExpl =
   "maps an IP address to a host name/service name";

NameToAddrText::NameToAddrText() :
   CliText(NameToAddrTextExpl, NameToAddrTextStr)
{
   BindParm(*new HostNameMandParm);
   BindParm(*new ServiceNameOptParm);
}

constexpr id_t LocalNameIndex = 1;
constexpr id_t UsesIPv6Index = 2;
constexpr id_t LocalAddrIndex = 3;
constexpr id_t LocalAddrsIndex = 4;
constexpr id_t NameToAddrIndex = 5;
constexpr id_t AddrToNameIndex = 6;

fixed_string IpActionExpl = "function to execute...";

IpAction::IpAction() : CliTextParm(IpActionExpl)
{
   BindText(*new CliText(LocalNameTextExpl, LocalNameTextStr), LocalNameIndex);
   BindText(*new CliText(UsesIPv6TextExpl, UsesIPv6TextStr), UsesIPv6Index);
   BindText(*new LocalAddrText, LocalAddrIndex);
   BindText(*new CliText
      (LocalAddrsTextExpl, LocalAddrsTextStr), LocalAddrsIndex);
   BindText(*new NameToAddrText, NameToAddrIndex);
   BindText(*new IpAddrParm
      (AddrToNameTextExpl, AddrToNameTextStr), AddrToNameIndex);
}

fixed_string IpStr = "ip";
fixed_string IpExpl = "Executes IP functions.";

IpCommand::IpCommand() : CliCommand(IpStr, IpExpl)
{
   BindParm(*new IpAction);
}

fn_name IpCommand_ProcessCommand = "IpCommand.ProcessCommand";

word IpCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft(IpCommand_ProcessCommand);

   id_t index;
   auto retest = false;
   string name, service;
   SysIpL3Addr host;
   IpProtocol proto;
   std::vector< SysIpL2Addr > localAddrs;

   if(!GetTextIndex(index, cli)) return -1;

   switch(index)
   {
   case LocalNameIndex:
      if(!cli.EndOfInput()) return -1;
      if(!SysIpL2Addr::LocalName(name)) return cli.Report(-2, NoHostNameExpl);
      return cli.Report(0, name);

   case UsesIPv6Index:
      if(!cli.EndOfInput()) return -1;
      *cli.obuf << spaces(2) << "Uses IPv6: ";
      *cli.obuf << IpPortRegistry::UseIPv6() << CRLF;
      break;

   case LocalAddrIndex:
      if(GetBoolParmRc(retest, cli) == Error) return -1;
      if(!cli.EndOfInput()) return -1;
      if(retest)
      {
         *cli.obuf << spaces(2) << "Retesting local address..." << CRLF;
         cli.Flush();
         Singleton< SendLocalThread >::Instance()->Retest();
         ThisThread::Pause(5 * ONE_SEC);
      }
      *cli.obuf << spaces(2) << "Local address: ";
      Singleton< IpPortRegistry >::Instance()->DisplayLocalAddr(*cli.obuf);
      *cli.obuf << CRLF;
      break;

   case LocalAddrsIndex:
      if(!cli.EndOfInput()) return -1;
      localAddrs = SysIpL2Addr::LocalAddrs();
      *cli.obuf << spaces(2) << "Local addresses:" << CRLF;

      if(localAddrs.empty())
      {
         *cli.obuf << spaces(4) << "None found." << CRLF;
      }
      else
      {
         for(size_t i = 0; i < localAddrs.size(); ++i)
         {
            *cli.obuf << spaces(4) << localAddrs[i].to_str() << CRLF;
         }
      }
      break;

   case NameToAddrIndex:
      if(!GetString(name, cli)) return -1;
      if(!GetString(service, cli)) service.clear();
      if(!cli.EndOfInput()) return -1;
      host = SysIpL3Addr(name, service, proto);
      if(!host.IsValid()) return cli.Report(-2, NoHostAddrExpl);
      *cli.obuf << spaces(2) << host.to_str(false) << CRLF;
      break;

   case AddrToNameIndex:
      if(!GetIpL3Addr(host, *this, cli)) return -1;
      if(!cli.EndOfInput()) return -1;
      if(!host.AddrToName(name, service)) return cli.Report(-2, NoHostInfoExpl);
      *cli.obuf << spaces(2) << name;
      if(!service.empty() && (service != "0")) *cli.obuf << " : " << service;
      *cli.obuf << CRLF;
      break;

   default:
      Debug::SwLog(IpCommand_ProcessCommand, UnexpectedIndex, index);
      return cli.Report(index, SystemErrorExpl);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The IPPORTS command.
//
class IpPortsCommand : public CliCommand
{
public:
   IpPortsCommand();
private:
   word ProcessCommand(CliThread& cli) const override;
};

fixed_string IpPortsStr = "ipports";
fixed_string IpPortsExpl = "Displays IP ports with input handlers.";

IpPortsCommand::IpPortsCommand() : CliCommand(IpPortsStr, IpPortsExpl)
{
   BindParm(*new IpPortOptParm);
   BindParm(*new DispBVParm);
}

word IpPortsCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("IpPortsCommand.ProcessCommand");

   word port;
   bool all, v = false;

   switch(GetIntParmRc(port, cli))
   {
   case None: all = true; break;
   case Ok: all = false; break;
   default: return -1;
   }

   if(GetBV(*this, cli, v) == Error) return -1;
   if(!cli.EndOfInput()) return -1;

   auto reg = Singleton< IpPortRegistry >::Instance();

   if(all)
   {
      reg->Output(*cli.obuf, 2, v);
   }
   else
   {
      auto ipport = reg->GetPort(port);
      if(ipport == nullptr) return cli.Report(-2, NoIpPortExpl);
      ipport->Output(*cli.obuf, 2, v);
   }

   return 0;
}

//------------------------------------------------------------------------------
//
//  The QUERY command.
//
NwQueryCommand::NwQueryCommand(bool bind) : QueryCommand(bind) { }

void NwQueryCommand::Patch(sel_t selector, void* arguments)
{
   QueryCommand::Patch(selector, arguments);
}

word NwQueryCommand::ProcessSubcommand(CliThread& cli, id_t index) const
{
   Debug::ft("NwQueryCommand.ProcessSubcommand");

   if(index != SelectionsIndex)
   {
      return QueryCommand::ProcessSubcommand(cli, index);
   }

   Singleton< NwTracer >::Instance()->QuerySelections(*cli.obuf);
   return 0;
}

//------------------------------------------------------------------------------
//
//  The STATUS command.
//
void NwStatusCommand::Patch(sel_t selector, void* arguments)
{
   StatusCommand::Patch(selector, arguments);
}

word NwStatusCommand::ProcessCommand(CliThread& cli) const
{
   Debug::ft("NwStatusCommand.ProcessCommand");

   StatusCommand::ProcessCommand(cli);

   *cli.obuf << CRLF;
   *cli.obuf << "IP PORT DISCARDS" << CRLF;

   bool one = false;

   auto& ports = Singleton< IpPortRegistry >::Instance()->Ports();

   for(auto p = ports.First(); p != nullptr; ports.Next(p))
   {
      auto msgs = p->Discards();

      if(msgs > 0)
      {
         if(!one) *cli.obuf << "   Msgs  IP Port" << CRLF;
         one = true;
         *cli.obuf << setw(7) << msgs;
         *cli.obuf << spaces(2) << p->GetService()->Name() << CRLF;
      }
   }

   if(!one) *cli.obuf << spaces(2) << NoDiscardsExpl << CRLF;

   return 0;
}

//------------------------------------------------------------------------------
//
//  The Network layer increment.
//
fixed_string NetworkText = "nw";
fixed_string NetworkExpl = "Network Increment";

NwIncrement::NwIncrement() : CliIncrement(NetworkText, NetworkExpl)
{
   Debug::ft("NwIncrement.ctor");

   BindCommand(*new IpCommand);
   BindCommand(*new IpPortsCommand);
   BindCommand(*new NwStatusCommand);
   BindCommand(*new NwIncludeCommand);
   BindCommand(*new NwExcludeCommand);
   BindCommand(*new NwQueryCommand);
   BindCommand(*new NwClearCommand);
}

//------------------------------------------------------------------------------

NwIncrement::~NwIncrement()
{
   Debug::ftnt("NwIncrement.dtor");
}
}
