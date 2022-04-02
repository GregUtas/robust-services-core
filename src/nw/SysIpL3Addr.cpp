//==============================================================================
//
//  SysIpL3Addr.cpp
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
#include "SysIpL3Addr.h"
#include <cctype>
#include <cstddef>
#include <iosfwd>
#include <sstream>
#include "Debug.h"
#include "Formatters.h"
#include "SysSocket.h"
#include "SysTcpSocket.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Parses the port (in decimal) that follows TEXT[INDEX] and returns it in
//  PORT.  Returns FALSE if a port does not follow TEXT[INDEX] or its value
//  is out of range.
//
static bool GetDecPort(const string& text, size_t& index, ipport_t& port)
{
   Debug::ft("NetworkBase.GetDecPort");

   auto found = false;
   uword value = 0;

   index = strSkipSpaces(text, index);
   port = 0;

   while(index < text.size())
   {
      if(!isdigit(text[index])) break;

      found = true;
      value = (value * 10) + (text[index++] - '0');
      if(value > MaxIpPort) return false;
   }

   port = ipport_t(value);
   return found;
}

//==============================================================================

SysIpL3Addr::SysIpL3Addr() :
   port_(NilIpPort),
   proto_(IpAny),
   socket_(nullptr)
{
   Debug::ftnt("SysIpL3Addr.ctor");
}

//------------------------------------------------------------------------------

SysIpL3Addr::SysIpL3Addr(const SysIpL2Addr& l2Addr, ipport_t port,
   IpProtocol proto, SysTcpSocket* socket) : SysIpL2Addr(l2Addr),
   port_(port),
   proto_(proto),
   socket_(socket)
{
   Debug::ft("SysIpL3Addr.ctor(L2addr)");

   if(socket_ != nullptr) proto_ = socket_->Protocol();
}

//------------------------------------------------------------------------------

SysIpL3Addr::SysIpL3Addr(IPv4Addr netaddr, ipport_t netport,
   IpProtocol proto, SysTcpSocket* socket) : SysIpL2Addr(netaddr),
   port_(ntohs(netport)),
   proto_(proto),
   socket_(socket)
{
   Debug::ft("SysIpL3Addr.ctor(IPv4)");

   if(socket_ != nullptr) proto_ = socket_->Protocol();
}

//------------------------------------------------------------------------------

SysIpL3Addr::SysIpL3Addr(const uint16_t netaddr[8], ipport_t netport,
   IpProtocol proto, SysTcpSocket* socket) : SysIpL2Addr(netaddr),
   port_(ntohs(netport)),
   proto_(proto),
   socket_(socket)
{
   Debug::ft("SysIpL3Addr.ctor(IPv6)");

   if(socket_ != nullptr) proto_ = socket_->Protocol();
}

//------------------------------------------------------------------------------

SysIpL3Addr::SysIpL3Addr(const std::string& text) : SysIpL2Addr(text),
   port_(NilIpPort),
   proto_(IpAny),
   socket_(nullptr)
{
   Debug::ft("SysIpL3Addr.ctor(string)");

   //  Check if SysIpL2Addr rejected TEXT.
   //
   if(!IsValid()) return;

   auto col = string::npos;

   if(text.find('.') != string::npos)
   {
      //  This should be an IPv4 address.  Extract any port number.
      //
      col = text.find(':');
   }
   else
   {
      //  This should be an IPv6 address.  If it contains a '[',
      //  see if a port number follows the ']'.
      //
      auto lb = text.find('[');

      if(lb != string::npos)
      {
         auto rb = text.find(']');

         if(rb == string::npos)
         {
            Nullify();
            return;
         }

         col = strSkipSpaces(text, rb + 1);

         if((col >= text.size()) || (text[col] != ':'))
         {
            Nullify();
            return;
         }
      }
   }

   //  If a port number was specified, extract it.
   //
   if(col != string::npos)
   {
      ipport_t port;

      if(!GetDecPort(text, ++col, port))
      {
         Nullify();
         return;
      }

      port_ = port;
   }
}

//------------------------------------------------------------------------------

SysIpL3Addr::~SysIpL3Addr()
{
   Debug::ftnt("SysIpL3Addr.dtor");
}

//------------------------------------------------------------------------------

void SysIpL3Addr::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   SysIpL2Addr::Display(stream, prefix, options);

   stream << prefix << "port   : " << port_ << CRLF;
   stream << prefix << "proto  : " << proto_ << CRLF;
   stream << prefix << "socket : " << socket_ << CRLF;
}

//------------------------------------------------------------------------------

void SysIpL3Addr::HostToNetwork(IPv4Addr& netaddr, ipport_t& netport) const
{
   Debug::ft("SysIpL3Addr.HostToNetwork(IPv4)");

   SysIpL2Addr::HostToNetwork(netaddr);
   netport = htons(port_);
}

//------------------------------------------------------------------------------

void SysIpL3Addr::HostToNetwork(uint16_t netaddr[8], ipport_t& netport) const
{
   Debug::ft("SysIpL3Addr.HostToNetwork(IPv6)");

   SysIpL2Addr::HostToNetwork(netaddr);
   netport = htons(port_);
}

//------------------------------------------------------------------------------

bool SysIpL3Addr::L2AddrMatches(const SysIpL2Addr& that) const
{
   Debug::ftnt("SysIpL3Addr.L2AddrMatches");

   return SysIpL2Addr::operator==(that);
}

//------------------------------------------------------------------------------

void SysIpL3Addr::NetworkToHost(IPv4Addr netaddr, ipport_t netport)
{
   Debug::ft("SysIpL3Addr.NetworkToHost(IPv4)");

   SysIpL2Addr::NetworkToHost(netaddr);
   port_ = ntohs(netport);
}

//------------------------------------------------------------------------------

void SysIpL3Addr::NetworkToHost(const uint16_t netaddr[8], ipport_t netport)
{
   Debug::ft("SysIpL3Addr.NetworkToHost(IPv6)");

   SysIpL2Addr::NetworkToHost(netaddr);
   port_ = ntohs(netport);
}

//------------------------------------------------------------------------------

void SysIpL3Addr::Nullify()
{
   Debug::ft("SysIpL3Addr.Nullify");

   ReleaseSocket();
   port_ = NilIpPort;
   proto_ = IpAny;
   SysIpL2Addr::Nullify();
}

//------------------------------------------------------------------------------

bool SysIpL3Addr::operator==(const SysIpL3Addr& that) const
{
   if(port_ != that.port_) return false;
   return SysIpL2Addr::operator==(that);
}

//------------------------------------------------------------------------------

bool SysIpL3Addr::operator!=(const SysIpL3Addr& that) const
{
   return !(*this == that);
}

//------------------------------------------------------------------------------

void SysIpL3Addr::ReleaseSocket()
{
   Debug::ft("SysIpL3Addr.ReleaseSocket");

   if(socket_ != nullptr)
   {
      socket_->Release();
      socket_ = nullptr;
   }
}

//------------------------------------------------------------------------------

void SysIpL3Addr::SetSocket(SysTcpSocket* socket)
{
   Debug::ft("SysIpL3Addr.SetSocket");

   socket_ = socket;
   if(socket_ != nullptr) proto_ = socket->Protocol();
}

//------------------------------------------------------------------------------

string SysIpL3Addr::to_str(bool verbose) const
{
   std::ostringstream stream;
   auto ipv6 = (Family() == IPv6);
   auto l3 = (port_ != NilIpPort);

   if(ipv6 && l3) stream << '[';
   stream << SysIpL2Addr::to_str();
   if(ipv6 && l3) stream << ']';
   if(l3) stream << ':' << port_;

   if(verbose)
   {
      if((proto_ != IpAny) || (socket_ != nullptr))
      {
         stream << " [" << proto_;
         if(socket_ != nullptr) stream << ", " << socket_;
         stream << ']';
      }
   }

   return stream.str();
}
}
