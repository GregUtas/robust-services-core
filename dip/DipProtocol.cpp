//==============================================================================
//
//  DipProtocol.cpp
//
//  Copyright (C) 2019  Greg Utas
//
//  Diplomacy AI Client - Part of the DAIDE project (www.daide.org.uk).
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
#include "DipProtocol.h"
#include <ostream>
#include <string>
#include "BotThread.h"
#include "CliText.h"
#include "Debug.h"
#include "Formatters.h"
#include "Memory.h"
#include "NbAppIds.h"
#include "Singleton.h"
#include "SysIpL3Addr.h"
#include "SysSocket.h"
#include "SysTcpSocket.h"
#include "Token.h"

using std::ostream;
using std::string;
using namespace NetworkBase;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
void DipHeader::Display(ostream& stream) const
{
   stream << "signal: " << int(signal) << CRLF;
   stream << "spare: " << int(spare) << CRLF;
   stream << "length: " << length << CRLF;
}

//------------------------------------------------------------------------------

void DipMessage::Display(ostream& stream) const
{
   switch(header.signal)
   {
   case IM_MESSAGE:
      reinterpret_cast< const IM_Message* >(this)->Display(stream);
      break;
   case RM_MESSAGE:
      reinterpret_cast< const RM_Message* >(this)->Display(stream);
      break;
   case DM_MESSAGE:
      reinterpret_cast< const DM_Message* >(this)->Display(stream);
      break;
   case FM_MESSAGE:
      reinterpret_cast< const FM_Message* >(this)->Display(stream);
      break;
   case EM_MESSAGE:
      reinterpret_cast< const EM_Message* >(this)->Display(stream);
      break;
   case BM_MESSAGE:
      reinterpret_cast< const BM_Message* >(this)->Display(stream);
      break;
   default:
      stream << "Unknown signal: " << header.signal << CRLF;
   }
}

//------------------------------------------------------------------------------

void IM_Message::Display(ostream& stream) const
{
   stream << "IM message" << CRLF;
   header.Display(stream);
   stream << "version: " << version << CRLF;
   stream << "magic_number: " << strHex(magic_number) << CRLF;
}

//------------------------------------------------------------------------------

void RM_Message::Display(ostream& stream) const
{
   stream << "RM message" << CRLF;
   header.Display(stream);
}

//------------------------------------------------------------------------------

void DM_Message::Display(ostream& stream) const
{
   stream << "DM message" << CRLF;
   header.Display(stream);
   stream << "tokens:" << CRLF;

   auto count = header.length >> 1;
   auto segs = count / 10;

   for(auto s = 0; s <= segs; ++s)
   {
      auto begin = s * 10;
      auto end = begin + 10;
      if(count < end) end = count;
      if(begin == end) break;

      for(auto t = begin; t < end; ++t)
      {
         stream << strHex(tokens[t], 4) << SPACE;
      }

      stream << CRLF;

      for(auto t = begin; t < end; ++t)
      {
         Token token(tokens[t]);
         auto str = token.to_str();
         auto size = str.size();
         auto pad = (size < 6 ? 6 - size : 0);
         if(pad > 0) str.insert(0, spaces(pad));
         stream << str << SPACE;
      }

      stream << CRLF;
   }
}

//------------------------------------------------------------------------------

void FM_Message::Display(ostream& stream) const
{
   stream << "FM message" << CRLF;
   header.Display(stream);
}

//------------------------------------------------------------------------------

void EM_Message::Display(ostream& stream) const
{
   stream << "EM message" << CRLF;
   header.Display(stream);
   stream << "error: " << error << CRLF;
}

//------------------------------------------------------------------------------

void BM_Message::Display(ostream& stream) const
{
   stream << "BM message" << CRLF;
   stream << "signal: " << int(header.signal) << CRLF;
   stream << "event: " << int(header.spare) << CRLF;
   stream << "length: " << header.length << CRLF;

   if(header.length > 0)
   {
      stream << "bytes: " << CRLF;
      strBytes(stream, spaces(2), &first_payload_byte, header.length);
   }
}

//==============================================================================

DipInputHandler::DipInputHandler(IpPort* port) : InputHandler(port)
{
   Debug::ft("DipInputHandler.ctor");
}

//------------------------------------------------------------------------------

IpBuffer* DipInputHandler::AllocBuff(const byte_t* source,
   size_t size, byte_t*& dest, size_t& rcvd, SysTcpSocket* socket) const
{
   Debug::ft("DipInputHandler.AllocBuff");

   IpBufferPtr buff(socket->AcquireIcMsg());

   if(buff == nullptr)
   {
      //  This is the beginning of a new message.  Find the PENDING number
      //  of BYTES that it will contain.  We can receive that many bytes,
      //  but SIZE may be smaller or larger when segmentation or bundling
      //  occurs.  The buffer that we allocate, however, will be able to
      //  hold the entire message, even if it is segmented.
      //
      auto header = reinterpret_cast< const DipHeader* >(source);
      size_t pending = DipHeaderSize + ntohs(header->length);
      rcvd = (pending < size ? pending : size);
      buff.reset(new DipIpBuffer(MsgIncoming, pending));
      dest = buff->PayloadPtr();
   }
   else
   {
      auto payload = buff->PayloadPtr();
      auto received = buff->PayloadSize();
      auto header = reinterpret_cast< const DipHeader* >(payload);
      auto pending = DipHeaderSize + header->length - received;
      rcvd = (pending < size ? pending : size);
      dest = payload + received;
   }

   return buff.release();
}

//------------------------------------------------------------------------------

byte_t* DipInputHandler::HostToNetwork
   (IpBuffer& buff, byte_t* src, size_t size) const
{
   Debug::ft("DipInputHandler.HostToNetwork");

   //  Some fields are byte-oriented, but most are 16 bits long and
   //  therefore need to be converted.  Conversion is done in place.
   //
   auto msg = reinterpret_cast< DipHeader* >(buff.PayloadPtr());

   switch(msg->signal)
   {
   case IM_MESSAGE:
   {
      auto im = reinterpret_cast< IM_Message* >(src);
      im->version = htons(im->version);
      im->magic_number = htons(im->magic_number);
      break;
   }

   case DM_MESSAGE:
   {
      auto dm = reinterpret_cast< DM_Message* >(src);
      size_t count = msg->length >> 1;
      for(size_t i = 0; i < count; ++i)
      {
         dm->tokens[i] = htons(dm->tokens[i]);
      }
      break;
   }

   case EM_MESSAGE:
   {
      auto em = reinterpret_cast< EM_Message* >(src);
      em->error = htons(em->error);
      break;
   }
   }

   msg->length = htons(msg->length);
   return src;
}

//------------------------------------------------------------------------------

void DipInputHandler::NetworkToHost
   (IpBuffer& buff, byte_t* dest, const byte_t* src, size_t size) const
{
   Debug::ft("DipInputHandler.NetworkToHost");

   //  Copy the entire message and then modify it in place, similar to
   //  HostToNetwork.  If this is the FIRST message, convert msg->length
   //  to host order.  If this is the last message (all bytes are present),
   //  convert the rest of the message now that it is ready for processing.
   //
   auto dipbuff = static_cast< DipIpBuffer* >(&buff);
   auto first = (dipbuff->PayloadSize() == 0);
   Memory::Copy(dest, src, size);
   dipbuff->BytesAdded(size);

   auto msg = reinterpret_cast< DipHeader* >(buff.PayloadPtr());
   if(first) msg->length = ntohs(msg->length);

   if(dipbuff->PayloadSize() < msg->length) return;

   switch(msg->signal)
   {
   case RM_MESSAGE:
   {
      auto rm = reinterpret_cast< RM_Message* >(msg);
      size_t count = msg->length / 6;
      for(size_t i = 0; i < count; ++i)
      {
         rm->pairs[i].token = ntohs(rm->pairs[i].token);
      }
      break;
   }

   case DM_MESSAGE:
   {
      auto dm = reinterpret_cast< DM_Message* >(msg);
      size_t count = msg->length >> 1;
      for(size_t i = 0; i < count; ++i)
      {
         dm->tokens[i] = ntohs(dm->tokens[i]);
      }
      break;
   }

   case EM_MESSAGE:
   {
      auto em = reinterpret_cast< EM_Message* >(msg);
      em->error = ntohs(em->error);
      break;
   }
   }
}

//------------------------------------------------------------------------------

void DipInputHandler::ReceiveBuff
   (IpBufferPtr& buff, size_t size, Faction faction) const
{
   Debug::ft("DipInputHandler.ReceiveBuff");

   //  If the message is not complete, return it to the socket to await
   //  more bytes instead of passing it to BotThread for processing.
   //
   DipIpBufferPtr dipbuff(static_cast< DipIpBuffer* >(buff.release()));

   auto payload = dipbuff->PayloadPtr();
   auto received = dipbuff->PayloadSize();
   auto header = reinterpret_cast< const DipHeader* >(payload);
   auto pending = DipHeaderSize + header->length - received;

   if(pending == 0)
   {
      Singleton< BotThread >::Instance()->QueueMsg(dipbuff);
   }
   else
   {
      //  ADDR has to be obtained before DIPBUFF becomes nullptr...
      //
      auto& addr = dipbuff->RxAddr();
      addr.GetSocket()->SetIcMsg(dipbuff.release());
   }
}

//------------------------------------------------------------------------------

void DipInputHandler::SocketFailed(SysSocket* socket) const
{
   Debug::ft("DipInputHandler.SocketFailed");

   //  Send a message to BotThread, informing it of the failure.
   //
   DipIpBufferPtr buff(new DipIpBuffer(MsgIncoming, DipHeaderSize));
   auto msg = reinterpret_cast< BM_Message* >(buff->PayloadPtr());
   msg->header.signal = BM_MESSAGE;
   msg->header.spare = SOCKET_FAILURE_EVENT;
   msg->header.length = 0;
   Singleton< BotThread >::Instance()->QueueMsg(buff);
}

//==============================================================================

BotTcpService::BotTcpService() : port_(NilIpPort)
{
   Debug::ft("BotTcpService.ctor");
}

//------------------------------------------------------------------------------

InputHandler* BotTcpService::CreateHandler(IpPort* port) const
{
   Debug::ft("BotTcpService.CreateHandler");

   return new DipInputHandler(port);
}

//------------------------------------------------------------------------------

fixed_string BotTcpServiceStr = "DAI/TCP";
fixed_string BotTcpServiceExpl = "Diplomacy AI Protocol";

CliText* BotTcpService::CreateText() const
{
   Debug::ft("BotTcpService.CreateText");

   return new CliText(BotTcpServiceExpl, BotTcpServiceStr);
}

//------------------------------------------------------------------------------

void BotTcpService::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   TcpIpService::Display(stream, prefix, options);

   stream << prefix << "port : " << port_ << CRLF;
}

//------------------------------------------------------------------------------

void BotTcpService::GetAppSocketSizes(size_t& rxSize, size_t& txSize) const
{
   Debug::ft("BotTcpService.GetAppSocketSizes");

   //  Setting txSize to 0 prevents buffering of outgoing messages.
   //
   rxSize = 2048;
   txSize = 0;
}

//------------------------------------------------------------------------------

ipport_t BotTcpService::Port() const { return ipport_t(port_); }

//==============================================================================

DipIpBuffer::DipIpBuffer(MsgDirection dir, size_t size) :
   IpBuffer(dir, 0, size),
   currSize_(dir == MsgOutgoing ? size : 0)
{
   Debug::ft("DipIpBuffer.ctor");
}

//------------------------------------------------------------------------------

DipIpBuffer::DipIpBuffer(const DipIpBuffer& that) : IpBuffer(that),
   currSize_(that.currSize_)
{
   Debug::ft("DipIpBuffer.ctor(copy)");
}

//------------------------------------------------------------------------------

DipIpBuffer::~DipIpBuffer()
{
   Debug::ftnt("DipIpBuffer.dtor");
}

//------------------------------------------------------------------------------

bool DipIpBuffer::AddBytes(const byte_t* source, size_t size, bool& moved)
{
   Debug::ft("DipIpBuffer.AddBytes");

   if(!IpBuffer::AddBytes(source, size, moved)) return false;
   if(source != nullptr) currSize_ += size;
   return true;
}

//------------------------------------------------------------------------------

void DipIpBuffer::BytesAdded(size_t size)
{
   Debug::ft("DipIpBuffer.BytesAdded");

   currSize_ += size;
}

//------------------------------------------------------------------------------

void DipIpBuffer::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   IpBuffer::Display(stream, prefix, options);

   stream << prefix << "currSize : " << currSize_ << CRLF;
}

//------------------------------------------------------------------------------

void* DipIpBuffer::operator new(size_t size)
{
   Debug::ft("DipIpBuffer.operator new");

   return Singleton< DipIpBufferPool >::Instance()->DeqBlock(size);
}

//==============================================================================

const size_t DipIpBufferPool::BlockSize = sizeof(DipIpBuffer);

//------------------------------------------------------------------------------

DipIpBufferPool::DipIpBufferPool() :
   ObjectPool(DipIpBufferObjPoolId, MemDynamic, BlockSize, "DipIpBuffers")
{
   Debug::ft("DipIpBufferPool.ctor");
}

//------------------------------------------------------------------------------

DipIpBufferPool::~DipIpBufferPool()
{
   Debug::ftnt("DipIpBufferPool.dtor");
}
}
