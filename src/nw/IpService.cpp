//==============================================================================
//
//  IpService.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "IpService.h"
#include <cstdint>
#include <sstream>
#include <string>
#include "Algorithms.h"
#include "Debug.h"
#include "FunctionGuard.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "IpServiceRegistry.h"
#include "Log.h"
#include "NwLogs.h"
#include "Singleton.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
const id_t IpService::MaxId = 255;

//------------------------------------------------------------------------------

IpService::IpService()
{
   Debug::ft("IpService.ctor");

   Singleton<IpServiceRegistry>::Instance()->BindService(*this);
}

//------------------------------------------------------------------------------

fn_name IpService_dtor = "IpService.dtor";

IpService::~IpService()
{
   Debug::ftnt(IpService_dtor);

   Debug::SwLog(IpService_dtor, UnexpectedInvocation, 0);
   Singleton<IpServiceRegistry>::Extant()->UnbindService(*this);
}

//------------------------------------------------------------------------------

ptrdiff_t IpService::CellDiff()
{
   uintptr_t local;
   auto fake = reinterpret_cast<const IpService*>(&local);
   return ptrdiff(&fake->sid_, fake);
}

//------------------------------------------------------------------------------

fn_name IpService_CreateHandler = "IpService.CreateHandler";

InputHandler* IpService::CreateHandler(IpPort* port) const
{
   Debug::ft(IpService_CreateHandler);

   Debug::SwLog(IpService_CreateHandler, strOver(this), port->GetPort());
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name IpService_CreatePort = "IpService.CreatePort";

IpPort* IpService::CreatePort(ipport_t pid)
{
   Debug::ft(IpService_CreatePort);

   Debug::SwLog(IpService_CreatePort, strOver(this), pid);
   return nullptr;
}

//------------------------------------------------------------------------------

fn_name IpService_CreateText = "IpService.CreateText";

CliText* IpService::CreateText() const
{
   Debug::ft(IpService_CreateText);

   Debug::SwLog(IpService_CreateText, strOver(this), sid_.GetId());
   return nullptr;
}

//------------------------------------------------------------------------------

void IpService::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "sid      : " << sid_.to_str() << CRLF;
   stream << prefix << "Name     : " << Name() << CRLF;
   stream << prefix << "Protocol : " << Protocol() << CRLF;
   stream << prefix << "Port     : " << Port() << CRLF;
   stream << prefix << "Enabled  : " << Enabled() << CRLF;
   stream << prefix << "Faction  : " << GetFaction() << CRLF;
   stream << prefix << "RxSize   : " << RxSize() << CRLF;
   stream << prefix << "TxSize   : " << TxSize() << CRLF;
}

//------------------------------------------------------------------------------

fn_name IpService_Enabled = "IpService.Enabled";

bool IpService::Enabled() const
{
   Debug::ft(IpService_Enabled);

   Debug::SwLog(IpService_Enabled, strOver(this), sid_.GetId());
   return false;
}

//------------------------------------------------------------------------------

fn_name IpService_GetAppSocketSizes = "IpService.GetAppSocketSizes";

void IpService::GetAppSocketSizes(size_t& rxSize, size_t& txSize) const
{
   Debug::ft(IpService_GetAppSocketSizes);

   rxSize = 0;
   txSize = 0;
   Debug::SwLog(IpService_GetAppSocketSizes, strOver(this), sid_.GetId());
}

//------------------------------------------------------------------------------

fn_name IpService_GetFaction = "IpService.GetFaction";

Faction IpService::GetFaction() const
{
   Debug::ft(IpService_GetFaction);

   Debug::SwLog(IpService_GetFaction, strOver(this), sid_.GetId());
   return OperationsFaction;
}

//------------------------------------------------------------------------------

fn_name IpService_HasSharedSocket = "IpService.HasSharedSocket";

bool IpService::HasSharedSocket() const
{
   Debug::ft(IpService_HasSharedSocket);

   Debug::SwLog(IpService_HasSharedSocket, strOver(this), sid_.GetId());
   return false;
}

//------------------------------------------------------------------------------

fn_name IpService_Name = "IpService.Name";

c_string IpService::Name() const
{
   Debug::ft(IpService_Name);

   Debug::SwLog(IpService_Name, strOver(this), sid_.GetId());
   return ERROR_STR;
}

//------------------------------------------------------------------------------

void IpService::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name IpService_Port = "IpService.Port";

ipport_t IpService::Port() const
{
   Debug::ft(IpService_Port);

   Debug::SwLog(IpService_Port, strOver(this), sid_.GetId());
   return NilIpPort;
}

//------------------------------------------------------------------------------

fn_name IpService_Protocol = "IpService.Protocol";

IpProtocol IpService::Protocol() const
{
   Debug::ft(IpService_Protocol);

   Debug::SwLog(IpService_Protocol, strOver(this), sid_.GetId());
   return IpAny;
}

//------------------------------------------------------------------------------

fn_name IpService_Provision = "IpService.Provision";

IpPort* IpService::Provision(ipport_t pid)
{
   Debug::ft(IpService_Provision);

   auto reg = Singleton<IpPortRegistry>::Instance();
   auto port = reg->GetPort(pid, Protocol());

   if(port != nullptr)
   {
      auto svc = port->GetService();
      if(svc != this)
      {
         auto log = Log::Create(NetworkLogGroup, NetworkPortOccupied);
         if(log != nullptr)
         {
            *log << Log::Tab << "port=" << pid << " errval=" << Name();
            Log::Submit(log);
         }
         return nullptr;
      }
      return port;
   }

   FunctionGuard guard(Guard_MemUnprotect);

   port = CreatePort(pid);
   if(port == nullptr)
   {
      string info(Name());
      info += " : failed to allocate IpPort";
      Debug::SwLog(IpService_Provision, info, pid);
      return nullptr;
   }

   auto handler = CreateHandler(port);
   if(handler == nullptr)
   {
      string info(Name());
      info += " : failed to allocate InputHandler";
      Debug::SwLog(IpService_Provision, info, pid);
      return nullptr;
   }

   return port;
}

//------------------------------------------------------------------------------

void IpService::Startup(RestartLevel level)
{
   Debug::ft("IpService.Startup");

   auto pid = Port();
   if(pid != NilIpPort) Provision(pid);
}
}
