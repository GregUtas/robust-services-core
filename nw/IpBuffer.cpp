//==============================================================================
//
//  IpBuffer.cpp
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
#include "IpBuffer.h"
#include <ostream>
#include <string>
#include <utility>
#include "Algorithms.h"
#include "AllocationException.h"
#include "ByteBuffer.h"
#include "Debug.h"
#include "Formatters.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "IpService.h"
#include "Memory.h"
#include "NwPools.h"
#include "NwTracer.h"
#include "Restart.h"
#include "Singleton.h"
#include "SysTcpSocket.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace NetworkBase
{
static std::unique_ptr< ByteBuffer > AllocByteBuff(size_t bytes)
{
   Debug::ft("NetworkBase.AllocByteBuff");

   if(bytes <= TinyBuffer::ArraySize)
      return std::unique_ptr< ByteBuffer >(new TinyBuffer);
   else if(bytes <= SmallBuffer::ArraySize)
      return std::unique_ptr< ByteBuffer >(new SmallBuffer);
   else if(bytes <= MediumBuffer::ArraySize)
      return std::unique_ptr< ByteBuffer >(new MediumBuffer);
   else if(bytes <= LargeBuffer::ArraySize)
      return std::unique_ptr< ByteBuffer >(new LargeBuffer);
   else if(bytes <= HugeBuffer::ArraySize)
      return std::unique_ptr< ByteBuffer >(new HugeBuffer);

   throw AllocationException(MemDynamic, bytes);
}

//==============================================================================

const size_t IpBuffer::MaxBuffSize = HugeBuffer::ArraySize;

//------------------------------------------------------------------------------

IpBuffer::IpBuffer(MsgDirection dir, size_t header, size_t payload) :
   MsgBuffer(),
   buffSize_(0),
   bytes_(nullptr),
   hdrSize_(header),
   dir_(dir),
   external_(false),
   queued_(false)
{
   Debug::ft("IpBuffer.ctor");

   AllocBuff(header + payload);
}

//------------------------------------------------------------------------------

IpBuffer::~IpBuffer()
{
   Debug::ftnt("IpBuffer.dtor");
}

//------------------------------------------------------------------------------

IpBuffer::IpBuffer(const IpBuffer& that) : MsgBuffer(that),
   buffSize_(0),
   bytes_(nullptr),
   hdrSize_(that.hdrSize_),
   txAddr_(that.txAddr_),
   rxAddr_(that.rxAddr_),
   dir_(that.dir_),
   external_(that.external_),
   queued_(false)
{
   Debug::ft("IpBuffer.ctor(copy)");

   //  Allocate a buffer and copy the original's contents into it.
   //
   AllocBuff(that.buffSize_);
   Memory::Copy(bytes_, that.bytes_, hdrSize_ + that.PayloadSize());
}

//------------------------------------------------------------------------------

bool IpBuffer::AddBytes(const byte_t* source, size_t size, bool& moved)
{
   Debug::ft("IpBuffer.AddBytes");

   //  If the buffer can't hold SIZE more bytes, extend its size.
   //
   moved = false;
   auto paySize = PayloadSize();
   auto newSize = hdrSize_ + paySize + size;

   if(newSize > buffSize_)
   {
      moved = AllocBuff(newSize);
   }

   //  Copy SIZE bytes into the buffer if they have been supplied.
   //
   if(source != nullptr)
   {
      Memory::Copy((bytes_ + hdrSize_ + paySize), source, size);
   }

   return true;
}

//------------------------------------------------------------------------------

bool IpBuffer::AllocBuff(size_t bytes)
{
   Debug::ft("IpBuffer.AllocBuff");

   if(bytes <= buffSize_) return false;

   auto newbuff = AllocByteBuff(bytes);
   auto newbytes = newbuff->Bytes();

   if(buff_ != nullptr)
   {
      Memory::Copy(newbytes, bytes_, hdrSize_ + PayloadSize());
   }

   buff_ = std::move(newbuff);
   buffSize_ = buff_->Size();
   bytes_ = (byte_t*) newbytes;
   return true;
}

//------------------------------------------------------------------------------

void IpBuffer::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   MsgBuffer::Display(stream, prefix, options);

   stream << prefix << "buff     : " << strPtr(buff_.get()) << CRLF;
   stream << prefix << "buffSize : " << buffSize_ << CRLF;
   stream << prefix << "bytes    : " << strPtr(bytes_) << CRLF;
   stream << prefix << "hdrSize  : " << hdrSize_ << CRLF;
   stream << prefix << "txAddr   : " << txAddr_.to_str(true) << CRLF;
   stream << prefix << "rxAddr   : " << rxAddr_.to_str(true) << CRLF;
   stream << prefix << "dir      : " << dir_ << CRLF;
   stream << prefix << "external : " << external_ << CRLF;
   stream << prefix << "queued   : " << queued_ << CRLF;
   stream << prefix << "length   : " << PayloadSize() << CRLF;

   strBytes(stream, prefix + spaces(2), bytes_, hdrSize_ + PayloadSize());
}

//------------------------------------------------------------------------------

TraceStatus IpBuffer::GetStatus() const
{
   return Singleton< NwTracer >::Instance()->BuffStatus(*this, dir_);
}

//------------------------------------------------------------------------------

void IpBuffer::GetSubtended(std::vector< Base* >& objects) const
{
   Debug::ft("IpBuffer.GetSubtended");

   Pooled::GetSubtended(objects);

   buff_->GetSubtended(objects);
}

//------------------------------------------------------------------------------

void IpBuffer::InvalidDiscarded() const
{
   Debug::ft("IpBuffer.InvalidDiscarded");

   auto reg = Singleton< IpPortRegistry >::Instance();
   auto port = reg->GetPort(rxAddr_.GetPort());
   if(port != nullptr) port->InvalidDiscarded();
}

//------------------------------------------------------------------------------

void* IpBuffer::operator new(size_t size)
{
   Debug::ft("IpBuffer.operator new");

   return Singleton< IpBufferPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

size_t IpBuffer::OutgoingBytes(byte_t*& bytes) const
{
   Debug::ft("IpBuffer.OutgoingBytes");

   if(external_)
   {
      bytes = bytes_ + hdrSize_;
      return PayloadSize();
   }

   bytes = bytes_;
   return hdrSize_ + PayloadSize();
}

//------------------------------------------------------------------------------

void IpBuffer::Patch(sel_t selector, void* arguments)
{
   MsgBuffer::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

size_t IpBuffer::Payload(byte_t*& bytes) const
{
   Debug::ft("IpBuffer.Payload");

   bytes = bytes_;
   if(bytes == nullptr) return 0;

   bytes += hdrSize_;
   return PayloadSize();
}

//------------------------------------------------------------------------------

size_t IpBuffer::PayloadSize() const
{
   Debug::ft("IpBuffer.PayloadSize");

   return buffSize_ - hdrSize_;
}

//------------------------------------------------------------------------------

fn_name IpBuffer_Send = "IpBuffer.Send";

bool IpBuffer::Send(bool external)
{
   Debug::ft(IpBuffer_Send);

   external_ = external;

   if(buff_ == nullptr)
   {
      Debug::SwLog(IpBuffer_Send, "null buffer", txAddr_.GetPort());
      return false;
   }

   //  An IpBuffer can be subclassed, so truncate the outgoing message to
   //  the current size of the application payload.  This prevents unused
   //  bytes from being sent if the buffer is queued for output, at which
   //  time it gets downclassed to an IpBuffer.
   //
   buffSize_ = hdrSize_ + PayloadSize();

   //  If there is a dedicated socket for the destination, send the message
   //  over it.  If not, find the IP service associated with the sender and
   //  see if it shares the I/O thread's primary socket (e.g. for UDP).
   //
   SysSocket* socket = rxAddr_.GetSocket();

   if(socket == nullptr)
   {
      auto txPort = txAddr_.GetPort();
      auto txProto = txAddr_.GetProtocol();
      auto reg = Singleton< IpPortRegistry >::Instance();
      auto ipPort = reg->GetPort(txPort, txProto);

      if(ipPort == nullptr)
      {
         Debug::SwLog(IpBuffer_Send, "port not found", txPort);
         return false;
      }

      auto svc = ipPort->GetService();

      if(svc == nullptr)
      {
         Debug::SwLog(IpBuffer_Send, "service not found", txPort);
         return false;
      }

      if(!svc->HasSharedSocket())
      {
         Debug::SwLog(IpBuffer_Send, "no shared socket", txPort);
         return false;
      }

      socket = ipPort->GetSocket();

      if(socket == nullptr)
      {
         if(Restart::GetStage() != ShuttingDown)
         {
            Debug::SwLog(IpBuffer_Send, "socket not found", txPort);
         }

         return false;
      }
   }

   return (socket->SendBuff(*this) != SysSocket::SendFailed);
}
}
