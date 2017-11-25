//==============================================================================
//
//  IpBuffer.h
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
#ifndef IPBUFFER_H_INCLUDED
#define IPBUFFER_H_INCLUDED

#include "MsgBuffer.h"
#include <cstddef>
#include "NbTypes.h"
#include "SysIpL3Addr.h"
#include "SysTypes.h"

//------------------------------------------------------------------------------

namespace NodeBase
{
//  IpBuffer wraps a message that passes between NodeBase and the IP stack.
//  It allocates a buffer for a contiguous message that may include an
//  internal header.
//
class IpBuffer : public MsgBuffer
{
public:
   //> The maximum number of bytes that can be added to an IpBuffer.
   //
   static const size_t MaxBuffSize;

   //  Allocates a buffer of size HEADER + PAYLOAD.  DIR specifies whether
   //  the buffer will receive or send a message.
   //
   IpBuffer(MsgDirection dir, MsgSize header, MsgSize payload);

   //  Copy constructor.
   //
   IpBuffer(const IpBuffer& that);

   //  Frees buff_.  Virtual to allow subclassing.
   //
   virtual ~IpBuffer();

   //  Returns the source IP address/port.
   //
   const SysIpL3Addr& TxAddr() const { return txAddr_; }

   //  Returns the destination IP address/port.
   //
   const SysIpL3Addr& RxAddr() const { return rxAddr_; }

   //  Returns the buffer's direction (incoming or outgoing).
   //
   MsgDirection Dir() const { return dir_; }

   //  Updates the buffer's direction (incoming or outgoing).
   //
   void SetDir(MsgDirection dir) { dir_ = dir; }

   //  Sets the destination IP address/port.  When using TCP, the socket
   //  bound to the host's ephemeral port is placed in rxAddr_.socket.
   //
   void SetRxAddr(const SysIpL3Addr& addr) { rxAddr_ = addr; }

   //  Sets the source IP address/port.
   //
   //  NOTE: The port must always be the well-known port for the IpService
   //  ====  that is sending the message, whether using TCP or UDP.  This
   //        is obviously for UDP but is also the case for TCP, even after
   //        Connect or Accept allocates an ephemeral port.  The ephemeral
   //        port is only identified indirectly, through its socket, which
   //        is actually placed in rxAddr_.socket (see above).
   //
   void SetTxAddr(const SysIpL3Addr& addr) { txAddr_ = addr; }

   //  Invoked when the buffer is queued for output.
   //
   void SetQueued() { queued_ = true; }

   //  Returns true if the buffer was queued for output.
   //
   bool IsQueued() const { return queued_; }

   //  Returns a pointer to the message header, which is also the start
   //  of the buffer.
   //
   byte_t* HeaderPtr() const { return buff_; }

   //  Returns a pointer to the payload, skipping the message header.
   //
   byte_t* PayloadPtr() const { return buff_ + hdrSize_; }

   //  Returns the number of bytes in the payload.  This may be less than
   //  what the buffer can actually hold, because outgoing messages are
   //  usually constructed by estimating the space that will be needed by
   //  the payload and then gradually filling it.  The default version
   //  returns the total buffer size minus the header size, which should
   //  be correct for an *incoming* message.
   //
   virtual MsgSize PayloadSize() const;

   //  Returns the number of bytes in the payload and updates BYTES to
   //  reference it.  The payload excludes the message header.
   //
   MsgSize Payload(byte_t*& bytes) const;

   //  Returns the number of bytes in an outgoing message and updates BYTES
   //  to reference it.  The size of the message, and where it starts, depend
   //  on whether it is being sent externally.
   //
   MsgSize OutgoingBytes(byte_t*& bytes) const;

   //  Adds SIZE bytes to the buffer, copying them from SOURCE.  If SOURCE
   //  is nullptr, nothing is copied into the buffer, but a larger buffer
   //  is obtained if SIZE more bytes will not fit into the current buffer.
   //  Returns true on success, setting MOVED if the location of the message
   //  changed as a result of obtaining a larger buffer.
   //
   virtual bool AddBytes(const byte_t* source, MsgSize size, bool& moved);

   //  Sends the message.  If EXTERNAL is true, the message header is dropped.
   //
   bool Send(bool external);

   //  Invoked when an incoming buffer is discarded.
   //
   void InvalidDiscarded() const;

   //  Overridden to determine if the message should be traced.
   //
   virtual TraceStatus GetStatus() const override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden for patching.
   //
   virtual void Patch(sel_t selector, void* arguments) override;
protected:
   //  Overridden to free buff_ during recovery.
   //
   virtual void Cleanup() override;
private:
   //  When buff_ is allocated, this function rounds nBytes off to a standard
   //  size.  An exception is thrown if nBytes is greater than MaxBuffSize.
   //
   static size_t BuffSize(size_t nBytes);

   //  The buffer that holds the message.
   //
   byte_t* buff_;

   //  The size of the header.
   //
   const MsgSize hdrSize_;

   //  The size of buff_ (which includes any header).
   //
   MsgSize buffSize_;

   //  The source IP address.
   //
   SysIpL3Addr txAddr_;

   //  The destination IP address.
   //
   SysIpL3Addr rxAddr_;

   //  Whether the buffer is incoming or outgoing.
   //
   MsgDirection dir_ : 8;

   //  Set if the buffer is being sent externally.
   //
   bool external_ : 8;

   //  Set if the buffer was queued for output.
   //
   bool queued_ : 8;
};
}
#endif
