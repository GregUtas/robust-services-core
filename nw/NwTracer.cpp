//==============================================================================
//
//  NwTracer.cpp
//
//  Copyright (C) 2017  Greg Utas
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

using std::ostream;

//------------------------------------------------------------------------------

namespace NetworkBase
{
fixed_string NetworkTraceToolName = "NetworkTracer";
fixed_string NetworkTraceToolExpl = "traces sockets";

class NetworkTraceTool : public Tool
{
   friend class Singleton< NetworkTraceTool >;
private:
   NetworkTraceTool() : Tool(NetworkTracer, 'n', true) { }
   virtual const char* Name() const override { return NetworkTraceToolName; }
   virtual const char* Expl() const override { return NetworkTraceToolExpl; }
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

fn_name NwTracer_ctor = "NwTracer.ctor";

NwTracer::NwTracer()
{
   Debug::ft(NwTracer_ctor);

   for(auto i = 0; i < MaxPeerEntries; ++i) peers_[i] = PeerFilter();
   for(auto i = 0; i < MaxPortEntries; ++i) ports_[i] = PortFilter();

   Singleton< NetworkTraceTool >::Instance();
}

//------------------------------------------------------------------------------

fn_name NwTracer_dtor = "NwTracer.dtor";

NwTracer::~NwTracer()
{
   Debug::ft(NwTracer_dtor);
}

//------------------------------------------------------------------------------

fn_name NwTracer_BuffStatus = "NwTracer.BuffStatus";

TraceStatus NwTracer::BuffStatus(const IpBuffer& ipb, MsgDirection dir) const
{
   Debug::ft(NwTracer_BuffStatus);

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
      Debug::SwErr(NwTracer_ClearSelections, filter, 0);
   }

   return TraceOk;
}

//------------------------------------------------------------------------------

fn_name NwTracer_FindPeer = "NwTracer.FindPeer";

int NwTracer::FindPeer(const SysIpL3Addr& peer) const
{
   Debug::ft(NwTracer_FindPeer);

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

fn_name NwTracer_FindPort = "NwTracer.FindPort";

int NwTracer::FindPort(ipport_t port) const
{
   Debug::ft(NwTracer_FindPort);

   for(auto i = 0; i < MaxPortEntries; ++i)
   {
      if(ports_[i].port == port) return i;
   }

   return -1;
}

//------------------------------------------------------------------------------

fn_name NwTracer_PeersEmpty = "NwTracer.PeersEmpty";

bool NwTracer::PeersEmpty() const
{
   Debug::ft(NwTracer_PeersEmpty);

   for(auto i = 0; i < MaxPeerEntries; ++i)
   {
      if(peers_[i].status != TraceDefault) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name NwTracer_PeerStatus = "NwTracer.PeerStatus";

TraceStatus NwTracer::PeerStatus(const SysIpL3Addr& peer) const
{
   Debug::ft(NwTracer_PeerStatus);

   auto i = FindPeer(peer);
   if(i < 0) return TraceDefault;
   return peers_[i].status;
}

//------------------------------------------------------------------------------

fn_name NwTracer_PortsEmpty = "NwTracer.PortsEmpty";

bool NwTracer::PortsEmpty() const
{
   Debug::ft(NwTracer_PortsEmpty);

   for(auto i = 0; i < MaxPortEntries; ++i)
   {
      if(ports_[i].status != TraceDefault) return false;
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name NwTracer_PortStatus = "NwTracer.PortStatus";

TraceStatus NwTracer::PortStatus(ipport_t port) const
{
   Debug::ft(NwTracer_PortStatus);

   auto i = FindPort(port);
   if(i < 0) return TraceDefault;
   return ports_[i].status;
}

//------------------------------------------------------------------------------

fixed_string PeersSelected = "Peers: ";
fixed_string PortsSelected = "Ports: ";

fn_name NwTracer_QuerySelections = "NwTracer.QuerySelections";

void NwTracer::QuerySelections(ostream& stream) const
{
   Debug::ft(NwTracer_QuerySelections);

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

fn_name NwTracer_SelectPeer = "NwTracer.SelectPeer";

TraceRc NwTracer::SelectPeer
   (const SysIpL3Addr& peer, TraceStatus status)
{
   Debug::ft(NwTracer_SelectPeer);

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

fn_name NwTracer_SelectPort = "NwTracer.SelectPort";

TraceRc NwTracer::SelectPort(ipport_t port, TraceStatus status)
{
   Debug::ft(NwTracer_SelectPort);

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
