//==============================================================================
//
//  SysUdpSocket.h
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
#ifndef SYSUDPSOCKET_H_INCLUDED
#define SYSUDPSOCKET_H_INCLUDED

#include "SysSocket.h"
#include <cstddef>
#include "NwTypes.h"
#include "SysTypes.h"

namespace NetworkBase
{
   class UdpIpService;
}

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  Operating system abstraction layer: UDP socket.
//
class SysUdpSocket : public SysSocket
{
public:
   //  Returns the maximum size of a UDP message (in bytes).
   //
   //  NOTE: This is obtained from getsockopt when the first UDP socket
   //  ====  is allocated.  Until that time, it is zero.  It is limited
   //        to SysSocket::MaxMsgSize.
   //
   static size_t MaxUdpSize() { return MaxUdpSize_; }

   //  Allocates a socket that will send and receive on PORT, on behalf
   //  of SERVICE.  RC is updated to indicate success or failure.
   //
   SysUdpSocket(ipport_t port, const UdpIpService* service, AllocRc& rc);

   //  Closes the socket.  Not subclassed.
   //
   ~SysUdpSocket();

   //  Reads up to SIZE bytes into BUFF.  Updates remAddr with the source of
   //  the bytes.  Returns the number of bytes read, or -1 on failure.  On
   //  an empty packet, returns 0.  If the socket is non-blocking, returns
   //  -2 if there was nothing to read.
   //
   NodeBase::word RecvFrom
      (NodeBase::byte_t* buff, size_t size, SysIpL3Addr& remAddr);

   //  Makes the socket non-blocking and sends DATA, of length SIZE, to the
   //  destination specified by remAddr. Returns the number of bytes sent,
   //  or -1 on failure.
   //
   NodeBase::word SendTo
      (const NodeBase::byte_t* data, size_t size, const SysIpL3Addr& remAddr);

   //  Overridden to indicate that this socket is running UDP.
   //
   IpProtocol Protocol() const override { return IpUdp; }

   //  Overridden to send BUFF.
   //
   SendRc SendBuff(IpBuffer& buff) override;

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  The maximum size of a UDP message.
   //
   static size_t MaxUdpSize_;
};
}
#endif
