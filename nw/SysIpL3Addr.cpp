//==============================================================================
//
//  SysIpL3Addr.cpp
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
#include "SysIpL3Addr.h"
#include <iosfwd>
#include <sstream>
#include "Debug.h"
#include "SysTcpSocket.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
SysIpL3Addr::SysIpL3Addr() :
   port_(NilIpPort),
   proto_(IpAny),
   socket_(nullptr)
{
   Debug::ft("SysIpL3Addr.ctor");
}

//------------------------------------------------------------------------------

SysIpL3Addr::SysIpL3Addr(ipv4addr_t v4Addr, ipport_t port,
   IpProtocol proto, SysTcpSocket* socket) : SysIpL2Addr(v4Addr),
   port_(port),
   proto_(proto),
   socket_(socket)
{
   Debug::ft("SysIpL3Addr.ctor(IPv4addr)");

   if(socket_ != nullptr) proto_ = socket_->Protocol();
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

bool SysIpL3Addr::operator==(const SysIpL3Addr& that) const
{
   return ((port_ == that.port_) && (GetIpV4Addr() == that.GetIpV4Addr()));
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

string SysIpL3Addr::to_str() const
{
   std::ostringstream stream;

   stream << SysIpL2Addr::to_str() << ": " << port_;
   return stream.str();
}

//------------------------------------------------------------------------------

string SysIpL3Addr::to_string() const
{
   std::ostringstream stream;

   stream << to_str();

   if((proto_ != IpAny) || (socket_ != nullptr))
   {
      stream << " [" << proto_ << ", " << socket_ << ']';
   }

   return stream.str();
}
}
