//==============================================================================
//
//  IpBuffer.h
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
#ifndef IPBUFFER_H_INCLUDED
#define IPBUFFER_H_INCLUDED

#include "MsgBuffer.h"
#include <cstddef>
#include <memory>
#include "NbTypes.h"
#include "SysIpL3Addr.h"
#include "SysTypes.h"

namespace NetworkBase
{
   class ByteBuffer;
}

//------------------------------------------------------------------------------

namespace NetworkBase
{
//  IpBuffer wraps a message that passes between an application and the IP
//  stack.  It allocates a buffer for a contiguous message that may include
//  an internal header.
//
class IpBuffer : public NodeBase::MsgBuffer
{
public:
   //> The maximum number of bytes that can be added to an IpBuffer.
   //
   static const size_t MaxBuffSize;

   //  Allocates a buffer of size HEADER + PAYLOAD.  DIR specifies whether
   //  the buffer will receive or send a message.
   //
   IpBuffer(NodeBase::MsgDirection dir, size_t header, size_t payload);

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
   NodeBase::MsgDirection Dir() const { return dir_; }

   //  Updates the buffer's direction (incoming or outgoing).
   //
   void SetDir(NodeBase::MsgDirection dir) { dir_ = dir; }

   //  Sets the destination IP address/port.  When using TCP, the socket
   //  dedicated to the connection must be placed in rxAddr_.socket.
   //
   void SetRxAddr(const SysIpL3Addr& addr) { rxAddr_ = addr; }

   //  Sets the source IP address/port.  The port must be the well-known
   //  port for the IpService that is sending the message.
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
   NodeBase::byte_t* HeaderPtr() const { return bytes_; }

   //  Returns a pointer to the payload, skipping the message header.
   //
   NodeBase::byte_t* PayloadPtr() const { return bytes_ + hdrSize_; }

   //  Returns the number of bytes in the payload.  The default version
   //  returns the total buffer size minus the header size, as it doesn't
   //  know how many bytes have been copied into the buffer.  An override
   //  will return the actual number of bytes in the payload, which will
   //  usually be less than what the buffer can hold.  The reason is that,
   //  internally, the buffer is allocated from an object pool, with there
   //  being enough pools to support a handful of sizes  This function is
   //  invoked by AddBytes (to see if a larger buffer should be allocated),
   //  OutgoingBytes (to provide a pointer to the message and return its
   //  size), and Send (to determine the number of bytes to send).
   //
   virtual size_t PayloadSize() const;

   //  Returns the number of bytes in the payload and updates BYTES to
   //  reference it.  The payload excludes the message header.
   //
   size_t Payload(NodeBase::byte_t*& bytes) const;

   //  Returns the number of bytes in an outgoing message and updates BYTES
   //  to reference it.  The size of the message, and where it starts, depend
   //  on whether it is being sent externally.
   //
   size_t OutgoingBytes(NodeBase::byte_t*& bytes) const;

   //  Adds SIZE bytes to the buffer, copying them from SOURCE.  If SOURCE
   //  is nullptr, nothing is copied into the buffer, but a larger buffer
   //  is obtained if SIZE more bytes will not fit into the current buffer.
   //  Returns true on success, setting MOVED if the location of the message
   //  changed as a result of obtaining a larger buffer.
   //
   virtual bool AddBytes
      (const NodeBase::byte_t* source, size_t size, bool& moved);

   //  Sends the message.  If EXTERNAL is true, the message header is dropped.
   //
   bool Send(bool external);

   //  Invoked when an incoming buffer is discarded.
   //
   void InvalidDiscarded() const;

   //  Overridden to obtain a buffer from its object pool.
   //
   static void* operator new(size_t size);

   //  Overridden to display member variables.
   //
   void Display(std::ostream& stream,
      const std::string& prefix, const NodeBase::Flags& options) const override;

   //  Overridden to determine if the message should be traced.
   //
   NodeBase::TraceStatus GetStatus() const override;

   //  Overridden to enumerate all objects that the buffer owns.
   //
   void GetSubtended(std::vector< Base* >& objects) const override;

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
private:
   //  Ensures that the byte buffer can hold SIZE bytes.  Returns true if a
   //  new buffer was allocated because the current one wasn't large enough.
   //
   bool AllocBuff(size_t bytes);

   //  The container allocated for the buffer's contents.
   //
   std::unique_ptr< ByteBuffer > buff_;

   //  The maximum number of bytes that buff_ can hold.
   //
   size_t buffSize_;

   //  The location of the buffer contents within buff_.
   //
   NodeBase::byte_t* bytes_;

   //  The size of the application header within buff_.
   //
   const size_t hdrSize_;

   //  The source IP address.
   //
   SysIpL3Addr txAddr_;

   //  The destination IP address.
   //
   SysIpL3Addr rxAddr_;

   //  Whether the buffer is incoming or outgoing.
   //
   NodeBase::MsgDirection dir_ : 8;

   //  Set if the buffer is being sent externally.
   //
   bool external_ : 8;

   //  Set if the buffer was queued for output.
   //
   bool queued_ : 8;
};
}
#endif
