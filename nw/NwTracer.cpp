//==============================================================================
//
//  NwTracer.cpp
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
#include "NwTracer.h"
#include "Tool.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "IpBuffer.h"
#include "NbTracer.h"
#include "Singleton.h"
#include "TraceBuffer.h"

using namespace NodeBase;
using std::ostream;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fixed_string NetworkTraceToolName = "NetworkTracer";
fixed_string NetworkTraceToolExpl = "traces sockets";

class NetworkTraceTool : public Tool
{
   friend class Singleton< NetworkTraceTool >;

   NetworkTraceTool() : Tool(NetworkTracer, 'n', true) { }
   ~NetworkTraceTool() = default;
   c_string Name() const override { return NetworkTraceToolName; }
   c_string Expl() const override { return NetworkTraceToolExpl; }
};

//------------------------------------------------------------------------------

NwTracer::PeerFilter::PeerFilter() :
   peer(SysIpL3Addr()), status(TraceDefault) { }

NwTracer::PeerFilter::PeerFilter(const SysIpL3Addr& a, TraceStatus s) :
   peer(a), status(s) { }

//------------------------------------------------------------------------------

NwTracer::PortFilter::PortFilter() :
   port(NilIpPort), status(TraceDefault) { }

NwTracer::PortFilter::PortFilter(ipport_t p, TraceStatus s) :
   port(p), status(s) { }

//------------------------------------------------------------------------------

NwTracer::NwTracer()
{
   Debug::ft("NwTracer.ctor");

   for(auto i = 0; i < MaxPeerEntries; ++i) peers_[i] = PeerFilter();
   for(auto i = 0; i < MaxPortEntries; ++i) ports_[i] = PortFilter();

   Singleton< NetworkTraceTool >::Instance();
}

//------------------------------------------------------------------------------

fn_name NwTracer_dtor = "NwTracer.dtor";

NwTracer::~NwTracer()
{
   Debug::ftnt(NwTracer_dtor);

   Debug::SwLog(NwTracer_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

TraceStatus NwTracer::BuffStatus(const IpBuffer& ipb, MsgDirection dir) const
{
   Debug::ft("NwTracer.BuffStatus");

   if(!Debug::TraceOn()) return TraceExcluded;

   auto buff = Singleton< TraceBuffer >::Instance();

   TraceStatus status;

   if(buff->FilterIsOn(TracePeer))
   {
      if(dir == MsgIncoming)
         status = PeerStatus(ipb.TxAddr());
      else
         status = PeerStatus(ipb.RxAddr());

      if(status != TraceDefault) return status;
   }

   if(buff->FilterIsOn(TracePort))
   {
      if(dir == MsgIncoming)
         status = PortStatus(ipb.RxAddr().GetPort());
      else
         status = PortStatus(ipb.TxAddr().GetPort());

      if(status != TraceDefault) return status;
   }

   return TraceDefault;
}

//------------------------------------------------------------------------------

fn_name NwTracer_ClearSelections = "NwTracer.ClearSelections";

TraceRc NwTracer::ClearSelections(FlagId filter)
{
   Debug::ft(NwTracer_ClearSelections);

   auto buff = Singleton< TraceBuffer >::Instance();

   switch(filter)
   {
   case TracePeer:
      for(auto i = 0; i < MaxPeerEntries; ++i) peers_[i] = PeerFilter();
      buff->ClearFilter(TracePeer);
      break;

   case TracePort:
      for(auto i = 0; i < MaxPortEntries; ++i) ports_[i] = PortFilter();
      buff->ClearFilter(TracePort);
      break;

   case TraceAll:
      Singleton< NbTracer >::Instance()->ClearSelections(TraceAll);
      ClearSelections(TracePeer);
      ClearSelections(TracePort);
      break;

   default:
      Debug::SwLog(NwTracer_ClearSelections, "unexpected filter", filter);
   }

   return TraceOk;
}

//------------------------------------------------------------------------------

int NwTracer::FindPeer(const SysIpL3Addr& peer) const
{
   Debug::ft("NwTracer.FindPeer");

   for(auto i = 0; i < MaxPeerEntries; ++i)
   {
      if(peers_[i].peer.GetIpV4Addr() == peer.GetIpV4Addr())
      {
         if(peers_[i].peer.GetPort() == peer.GetPort()) return i;
         if(peers_[i].peer.GetPort() == NilIpPort) return i;
      }
   }

   return -1;
}

//------------------------------------------------------------------------------

int NwTracer::FindPort(ipport_t port) const
{
   Debug::ft("NwTracer.FindPort");

   for(auto i = 0; i < MaxPortEntries; ++i)
   {
      if(ports_[i].port == port) return i;
   }

   return -1;
}

//------------------------------------------------------------------------------

void NwTracer::Patch(sel_t selector, void* arguments)
{
   Permanent::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

bool NwTracer::PeersEmpty() const
{
   Debug::ft("NwTracer.PeersEmpty");

   for(auto i = 0; i < MaxPeerEntries; ++i)
   {
      if(peers_[i].status != TraceDefault) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

TraceStatus NwTracer::PeerStatus(const SysIpL3Addr& peer) const
{
   Debug::ft("NwTracer.PeerStatus");

   auto i = FindPeer(peer);
   if(i < 0) return TraceDefault;
   return peers_[i].status;
}

//------------------------------------------------------------------------------

bool NwTracer::PortsEmpty() const
{
   Debug::ft("NwTracer.PortsEmpty");

   for(auto i = 0; i < MaxPortEntries; ++i)
   {
      if(ports_[i].status != TraceDefault) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

TraceStatus NwTracer::PortStatus(ipport_t port) const
{
   Debug::ft("NwTracer.PortStatus");

   auto i = FindPort(port);
   if(i < 0) return TraceDefault;
   return ports_[i].status;
}

//------------------------------------------------------------------------------

fixed_string PeersSelected = "Peers: ";
fixed_string PortsSelected = "Ports: ";

void NwTracer::QuerySelections(ostream& stream) const
{
   Debug::ft("NwTracer.QuerySelections");

   auto nbt = Singleton< NbTracer >::Instance();

   nbt->QuerySelections(stream);

   auto buff = Singleton< TraceBuffer >::Instance();

   stream << PeersSelected << CRLF;

   if(!buff->FilterIsOn(TracePeer))
   {
      stream << spaces(2) << TraceBuffer::NoneSelected << CRLF;
   }
   else
   {
      for(auto i = 0; i < MaxPeerEntries; ++i)
      {
         if(peers_[i].status != TraceDefault)
         {
            stream << spaces(2) << peers_[i].status << ": ";
            stream << peers_[i].peer.to_string() << CRLF;
         }
      }
   }

   stream << PortsSelected << CRLF;

   if(!buff->FilterIsOn(TracePort))
   {
      stream << spaces(2) << TraceBuffer::NoneSelected << CRLF;
   }
   else
   {
      for(auto i = 0; i < MaxPortEntries; ++i)
      {
         if(ports_[i].status != TraceDefault)
         {
            stream << spaces(2) << ports_[i].status << ": ";
            stream << ports_[i].port << CRLF;
         }
      }
   }
}

//------------------------------------------------------------------------------

TraceRc NwTracer::SelectPeer
   (const SysIpL3Addr& peer, TraceStatus status)
{
   Debug::ft("NwTracer.SelectPeer");

   auto buff = Singleton< TraceBuffer >::Instance();

   auto i = FindPeer(peer);

   if(i >= 0)
   {
      if(status == TraceDefault)
      {
         peers_[i] = PeerFilter();
         if(PeersEmpty()) buff->ClearFilter(TracePeer);
      }
      else
      {
         peers_[i].status = status;
      }

      return TraceOk;
   }

   if(status == TraceDefault) return TraceOk;

   i = FindPeer(SysIpL3Addr());
   if(i < 0) return RegistryIsFull;
   peers_[i] = PeerFilter(peer, status);
   buff->SetFilter(TracePeer);
   return TraceOk;
}

//------------------------------------------------------------------------------

TraceRc NwTracer::SelectPort(ipport_t port, TraceStatus status)
{
   Debug::ft("NwTracer.SelectPort");

   auto buff = Singleton< TraceBuffer >::Instance();

   auto i = FindPort(port);

   if(i >= 0)
   {
      if(status == TraceDefault)
      {
         ports_[i] = PortFilter();
         if(PortsEmpty()) buff->ClearFilter(TracePort);
      }
      else
      {
         ports_[i].status = status;
      }

      return TraceOk;
   }

   if(status == TraceDefault) return TraceOk;

   i = FindPort(NilIpPort);
   if(i < 0) return RegistryIsFull;
   ports_[i] = PortFilter(port, status);
   buff->SetFilter(TracePort);
   return TraceOk;
}
}
