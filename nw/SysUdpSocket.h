//==============================================================================
//
//  SysUdpSocket.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef SYSUDPSOCKET_H_INCLUDED
#define SYSUDPSOCKET_H_INCLUDED

#include "SysSocket.h"
#include <cstddef>
#include "NwTypes.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
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

   //  Allocates a socket that will send and receive on PORT.  RXSIZE and
   //  TXSIZE specify the size of the socket's receive and send buffers.
   //  RC is updated to indicate success or failure.
   //
   SysUdpSocket(ipport_t port, size_t rxSize, size_t txSize, AllocRc& rc);

   //  Closes the socket.  Not subclassed.
   //
   ~SysUdpSocket();

   //  Reads up to MAX bytes into BUFF.  Updates remAddr with the source of
   //  the bytes.  Returns the number of bytes read, or -1 on failure.  On
   //  an empty packet, returns 0.  If the socket is non-blocking, returns
   //  -2 if there was nothing to read.
   //
   word RecvFrom(byte_t* buff, size_t max, SysIpL3Addr& remAddr);

   //  Makes the socket non-blocking and sends DATA, of length LEN, to the
   //  destination specified by remAddr. Returns the number of bytes sent,
   //  or -1 on failure.
   //
   word SendTo(const byte_t* data, size_t len, const SysIpL3Addr& remAddr);

   //  Overridden to indicate that this socket is running UDP.
   //
   virtual IpProtocol Protocol() const override { return IpUdp; }

   //  Overridden to send BUFF.
   //
   virtual SendRc SendBuff(IpBuffer& buff) override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
private:
   //  The maximum size of a UDP message.
   //
   static size_t MaxUdpSize_;
};
}
#endif
