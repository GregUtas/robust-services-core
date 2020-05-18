//==============================================================================
//
//  IpBuffer.cpp
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
#include "IpBuffer.h"
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "IpPort.h"
#include "IpPortRegistry.h"
#include "IpService.h"
#include "Memory.h"
#include "NbAppIds.h"
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
//  The maximum size and the standard sizes for message buffers.  A small number
//  of fixed sizes reduces heap fragmentation.  It also leaves slack space that
//  reduces the odds of Memory::Realloc having to allocate a new buffer.
//
const size_t IpBuffer::MaxBuffSize = 8192;

const size_t nSizes = 5;

const size_t BuffSizes[nSizes + 1] =
{
   32, 128, 512, 2048, IpBuffer::MaxBuffSize
};

//------------------------------------------------------------------------------

fn_name IpBuffer_ctor1 = "IpBuffer.ctor";

IpBuffer::IpBuffer(MsgDirection dir, size_t header, size_t payload) :
   MsgBuffer(),
   buff_(nullptr),
   hdrSize_(header),
   buffSize_(header + payload),
   dir_(dir),
   external_(false),
   queued_(false)
{
   Debug::ft(IpBuffer_ctor1);

   buff_ = (byte_t*) Memory::Alloc(BuffSize(buffSize_), MemDynamic);
}

//------------------------------------------------------------------------------

fn_name IpBuffer_ctor2 = "IpBuffer.ctor(copy)";

IpBuffer::IpBuffer(const IpBuffer& that) : MsgBuffer(that),
   buff_(nullptr),
   hdrSize_(that.hdrSize_),
   buffSize_(that.buffSize_),
   txAddr_(that.txAddr_),
   rxAddr_(that.rxAddr_),
   dir_(that.dir_),
   external_(that.external_),
   queued_(false)
{
   Debug::ft(IpBuffer_ctor2);

   buff_ = (byte_t*) Memory::Alloc(BuffSize(buffSize_), MemDynamic);

   //  Copy the original buffer into the new one.
   //
   Memory::Copy(buff_, that.buff_, buffSize_);
}

//------------------------------------------------------------------------------

fn_name IpBuffer_dtor = "IpBuffer.dtor";

IpBuffer::~IpBuffer()
{
   Debug::ftnt(IpBuffer_dtor);

   if(buff_ != nullptr)
   {
      Memory::Free(buff_, MemDynamic);
      buff_ = nullptr;
   }
}

//------------------------------------------------------------------------------

fn_name IpBuffer_AddBytes = "IpBuffer.AddBytes";

bool IpBuffer::AddBytes(const byte_t* source, size_t size, bool& moved)
{
   Debug::ft(IpBuffer_AddBytes);

   //  If the buffer can't hold SIZE more bytes, extend its size.
   //
   moved = false;
   if(buff_ == nullptr) return false;

   auto paySize = PayloadSize();
   auto newSize = hdrSize_ + paySize + size;

   if(newSize > buffSize_)
   {
      auto buff = (byte_t*)
         Memory::Realloc(buff_, BuffSize(newSize), MemDynamic);
      if(buff == nullptr) return false;

      moved = (buff != buff_);
      buff_ = buff;
      buffSize_ = newSize;
   }

   //  Copy SIZE bytes into the buffer if they have been supplied.
   //
   if(source != nullptr)
   {
      Memory::Copy((buff_ + hdrSize_ + paySize), source, size);
   }

   return true;
}

//------------------------------------------------------------------------------

fn_name IpBuffer_BuffSize = "IpBuffer.BuffSize";

size_t IpBuffer::BuffSize(size_t size)
{
   Debug::ft(IpBuffer_BuffSize);

   for(auto i = 0; i <= nSizes; ++i)
   {
      if(BuffSizes[i] >= size) return BuffSizes[i];
   }

   Debug::SwErr("size out of range", size);
   return 0;
}

//------------------------------------------------------------------------------

fn_name IpBuffer_Cleanup = "IpBuffer.Cleanup";

void IpBuffer::Cleanup()
{
   Debug::ft(IpBuffer_Cleanup);

   if(buff_ != nullptr)
   {
      Memory::Free(buff_, MemDynamic);
      buff_ = nullptr;
   }

   MsgBuffer::Cleanup();
}

//------------------------------------------------------------------------------

void IpBuffer::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   MsgBuffer::Display(stream, prefix, options);

   stream << prefix << "buff     : " << strPtr(buff_) << CRLF;
   stream << prefix << "hdrSize  : " << hdrSize_ << CRLF;
   stream << prefix << "buffSize : " << buffSize_ << CRLF;
   stream << prefix << "txAddr   : " << txAddr_.to_string() << CRLF;
   stream << prefix << "rxAddr   : " << rxAddr_.to_string() << CRLF;
   stream << prefix << "dir      : " << dir_ << CRLF;
   stream << prefix << "external : " << external_ << CRLF;
   stream << prefix << "queued   : " << queued_ << CRLF;
   stream << prefix << "length   : " << PayloadSize() << CRLF;

   strBytes(stream, prefix + spaces(2), buff_, hdrSize_ + PayloadSize());
}

//------------------------------------------------------------------------------

TraceStatus IpBuffer::GetStatus() const
{
   return Singleton< NwTracer >::Instance()->BuffStatus(*this, dir_);
}

//------------------------------------------------------------------------------

fn_name IpBuffer_InvalidDiscarded = "IpBuffer.InvalidDiscarded";

void IpBuffer::InvalidDiscarded() const
{
   Debug::ft(IpBuffer_InvalidDiscarded);

   auto reg = Singleton< IpPortRegistry >::Instance();
   auto port = reg->GetPort(rxAddr_.GetPort());
   if(port != nullptr) port->InvalidDiscarded();
}

//------------------------------------------------------------------------------

fn_name IpBuffer_new = "IpBuffer.operator new";

void* IpBuffer::operator new(size_t size)
{
   Debug::ft(IpBuffer_new);

   return Singleton< IpBufferPool >::Instance()->DeqBlock(size);
}

//------------------------------------------------------------------------------

fn_name IpBuffer_OutgoingBytes = "IpBuffer.OutgoingBytes";

size_t IpBuffer::OutgoingBytes(byte_t*& bytes) const
{
   Debug::ft(IpBuffer_OutgoingBytes);

   if(external_)
   {
      bytes = buff_ + hdrSize_;
      return PayloadSize();
   }

   bytes = buff_;
   return hdrSize_ + PayloadSize();
}

//------------------------------------------------------------------------------

void IpBuffer::Patch(sel_t selector, void* arguments)
{
   MsgBuffer::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fn_name IpBuffer_Payload = "IpBuffer.Payload";

size_t IpBuffer::Payload(byte_t*& bytes) const
{
   Debug::ft(IpBuffer_Payload);

   bytes = buff_;
   if(bytes == nullptr) return 0;

   bytes += hdrSize_;
   return PayloadSize();
}

//------------------------------------------------------------------------------

fn_name IpBuffer_PayloadSize = "IpBuffer.PayloadSize";

size_t IpBuffer::PayloadSize() const
{
   Debug::ft(IpBuffer_PayloadSize);

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

//==============================================================================

const size_t IpBufferPool::BlockSize = sizeof(IpBuffer);

//------------------------------------------------------------------------------

fn_name IpBufferPool_ctor = "IpBufferPool.ctor";

IpBufferPool::IpBufferPool() :
   ObjectPool(IpBufferObjPoolId, MemDynamic, BlockSize, "IpBuffers")
{
   Debug::ft(IpBufferPool_ctor);
}

//------------------------------------------------------------------------------

fn_name IpBufferPool_dtor = "IpBufferPool.dtor";

IpBufferPool::~IpBufferPool()
{
   Debug::ftnt(IpBufferPool_dtor);
}
}
