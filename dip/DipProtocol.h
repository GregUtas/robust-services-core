//==============================================================================
//
//  DipProtocol.h
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
#ifndef DIPPROTOCOL_H_INCLUDED
#define DIPPROTOCOL_H_INCLUDED

#include "InputHandler.h"
#include "IpBuffer.h"
#include "ObjectPool.h"
#include "TcpIpService.h"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include "DipTypes.h"
#include "NbTypes.h"
#include "NwTypes.h"
#include "SysTypes.h"

using namespace NodeBase;
using namespace NetworkBase;

//------------------------------------------------------------------------------

namespace Diplomacy
{
//  Header for all client-server messages.
//
enum MsgType : uint8_t
{
   IM_MESSAGE,  // Initial Message
   RM_MESSAGE,  // Representation Message
   DM_MESSAGE,  // Diplomacy Message
   FM_MESSAGE,  // Final Message
   EM_MESSAGE,  // Error Message
   BM_MESSAGE   // Bot Message
};

struct DipHeader
{
   MsgType signal;   // type of message
   uint8_t spare;    // ignored except in BM_MESSAGE (see below)
   uint16_t length;  // number of bytes of data that follow

   //  Displays the header in STREAM.
   //
   void Display(std::ostream& stream) const;
};

constexpr uint16_t DipHeaderSize = sizeof(DipHeader);

struct DipMessage
{
   DipHeader header;
   byte_t first_payload_byte;  // for creating a pointer to the first byte

   //  Displays the message in STREAM.
   //
   void Display(std::ostream& stream) const;
};

//------------------------------------------------------------------------------
//
//  The client sends an IM message to the server as soon as the connection
//  is open.  It is only sent once.
//
struct IM_Message
{
   DipHeader header;
   uint16_t version;       // protocol version number
   uint16_t magic_number;  // to verify that client is using this protocol

   //  Displays the message in STREAM.
   //
   void Display(std::ostream& stream) const;
};

//------------------------------------------------------------------------------
//
//  The server sends an RM message to the client immediately after it receives
//  the IM.  If the length is zero, then the powers and provinces are those on
//  the standard map.
//
struct TokenName
{
   token_t token;       // power or province token
   const char name[4];  // 3-letter null-terminated name (A-Z and 0-9 only)
};

struct RM_Message
{
   const DipHeader header;
   TokenName pairs[POWER_MAX + PROVINCE_MAX];  // power & province names

   //  Displays the message in STREAM.
   //
   void Display(std::ostream& stream) const;
};

//------------------------------------------------------------------------------
//
//  All other messages between client and server, except for the last one,
//  are DM messages.  These messages contain tokens defined by the Diplomacy
//  language (see Token.h).  The message's raw length is always a multiple
//  of 2, which corresponds to twice the number of tokens in the message.
//
struct DM_Message
{
   DipHeader header;
   token_t tokens[UINT16_MAX >> 1];  // from 0 to (header_.length_ >> 1) - 1

   //  Displays the message in STREAM.
   //
   void Display(std::ostream& stream) const;
};

//------------------------------------------------------------------------------
//
//  The client or server sends an FM message immediately before closing the
//  connection.  The recipient then closes the connection without sending any
//  further message.
//
struct FM_Message
{
   DipHeader header;  // no parameters (length = 0)

   //  Displays the message in STREAM.
   //
   void Display(std::ostream& stream) const;
};

//------------------------------------------------------------------------------
//
//  The client or server sends an EM message immediately before closing the
//  connection because of an error.  The recipient then closes the connection
//  without sending any further message.  The only ProtocolError that BaseBot
//  sends is GRACEFUL_CLOSE; it doesn't disconnect if the server violates the
//  protocol.
//
typedef uint16_t ProtocolError;

constexpr ProtocolError GRACEFUL_CLOSE = 0x00;
constexpr ProtocolError IM_TIMEOUT = 0x01;
constexpr ProtocolError IM_EXPECTED = 0x02;
constexpr ProtocolError IM_WRONG_ENDIAN = 0x03;
constexpr ProtocolError IM_WRONG_MAGIC_NUMBER = 0x04;
constexpr ProtocolError IM_INCOMPATIBLE_VERSION = 0x05;
constexpr ProtocolError IM_REPEATED = 0x06;
constexpr ProtocolError IM_FROM_SERVER = 0x07;
constexpr ProtocolError INVALID_MESSAGE_TYPE = 0x08;
constexpr ProtocolError MESSAGE_TOO_SHORT = 0x09;
constexpr ProtocolError DM_BEFORE_RM = 0x0A;
constexpr ProtocolError RM_EXPECTED = 0x0B;
constexpr ProtocolError RM_REPEATED = 0x0C;
constexpr ProtocolError RM_FROM_CLIENT = 0x0D;
constexpr ProtocolError DM_INVALID_TOKEN = 0x0E;
constexpr ProtocolError SERVER_OFF = 0x20;         // OFF message from server
constexpr ProtocolError SOCKET_FAILED = 0x21;      // for internal use

struct EM_Message
{
   DipHeader header;
   ProtocolError error;

   //  Displays the message in STREAM.
   //
   void Display(std::ostream& stream) const;
};

//------------------------------------------------------------------------------
//
//  This message is used to receive internal events.  Anything that follows
//  the header is bot-specific, and header.spare is used to specify the event
//  that distinguishes various types of BM_Message.
//
constexpr BotEvent SOCKET_FAILURE_EVENT = 0;
constexpr BotEvent RECONNECT_EVENT = 1;
constexpr BotEvent FIRST_BOT_BM_EVENT = 2;  // start of subclass events

struct BM_Message
{
   DipHeader header;
   byte_t first_payload_byte;  // for creating a pointer to the first byte

   //  Displays the message in STREAM.
   //
   void Display(std::ostream& stream) const;
};

//------------------------------------------------------------------------------
//
//  Diplomacy protocol over TCP.
//
class BotTcpService : public TcpIpService
{
   friend class Singleton< BotTcpService >;
public:
   //  Overridden to return the service's attributes.
   //
   virtual const char* Name() const override { return "Diplomacy"; }
   virtual ipport_t Port() const override;
   virtual Faction GetFaction() const override { return PayloadFaction; }
   virtual bool AcceptsConns() const override { return false; }
   virtual size_t MaxConns() const override { return 4; }
   virtual size_t MaxBacklog() const override { return 0; }
   virtual bool Keepalive() const override { return true; }

   //  Sets the service's port number.
   //
   void SetPort(ipport_t port) { port_ = port; }

   //  Overridden to display the service's data.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;
private:
   //  Private because this singleton is not subclassed.
   //
   BotTcpService();

   //  Private because this singleton is not subclassed.
   //
   ~BotTcpService() = default;

   //  Overridden to return the socket's buffer sizes.
   //
   virtual void GetAppSocketSizes
      (size_t& rxSize, size_t& txSize) const override;

   //  Overridden to create a CLI parameter that identifies the protocol.
   //
   virtual CliText* CreateText() const override;

   //  Overridden to create the Diplomacy input handler.
   //
   virtual InputHandler* CreateHandler(IpPort* port) const override;

   //  The port on which the protocol is running.
   //
   word port_;
};

//------------------------------------------------------------------------------
//
//  Input handler for messages that contain a Diplomacy header.
//
class DipInputHandler : public InputHandler
{
public:
   //  Registers the input handler with PORT.
   //
   explicit DipInputHandler(IpPort* port);

   //  Not subclassed.
   //
   ~DipInputHandler() = default;

   //  Overridden to allocate a DipIpBuffer for a Diplomacy message that has
   //  received over TCP.
   //
   virtual IpBuffer* AllocBuff(const byte_t* source, size_t size,
      byte_t*& dest, size_t& rcvd, SysTcpSocket* socket) const override;

   //  Overridden to convert an incoming message from network to host order.
   //
   virtual void NetworkToHost(IpBuffer& buff,
      byte_t* dest, const byte_t* src, size_t size) const override;

   //  Overridden to queue an incoming message for BotThread.
   //
   virtual void ReceiveBuff
      (IpBufferPtr& buff, size_t size, Faction faction) const override;

   //  Overridden to convert an outgiong message from host to network order.
   //
   virtual byte_t* HostToNetwork(IpBuffer& buff,
      byte_t* src, size_t size) const override;

   //  Overridden to queue an incoming message for BotThread.
   //
   virtual void SocketFailed(SysSocket* socket) const override;
};

//------------------------------------------------------------------------------
//
//  IP buffer for sending and receiving Diplomacy messages.
//
class DipIpBuffer : public IpBuffer
{
public:
   //  Allocates a message for SIZE bytes that will travel in DIR.  If DIR
   //  is MsgOutgoing, the size of the payload (currSize_) is set to SIZE.
   //
   DipIpBuffer(MsgDirection dir, size_t size);

   //  Copy constructor.
   //
   DipIpBuffer(const DipIpBuffer& that);

   //  Not subclassed.
   //
   ~DipIpBuffer();

   //  Invoked after copying SIZE bytes into the message.
   //
   void BytesAdded(size_t size);

   //  Overridden to return the number of bytes currently in the message.
   //
   virtual size_t PayloadSize() const override { return currSize_; }

   //  Overridden to track the number of bytes currently in the message.
   //
   virtual bool AddBytes
      (const byte_t* source, size_t size, bool& moved) override;

   //  Overridden to display member variables.
   //
   virtual void Display(std::ostream& stream,
      const std::string& prefix, const Flags& options) const override;

   //  Overridden to obtain a buffer from its object pool.
   //
   static void* operator new(size_t size);
private:
   //  The number of bytes currently in the message.
   //
   size_t currSize_;
};

//------------------------------------------------------------------------------
//
//  Pool for DipIpBuffer objects.
//
class DipIpBufferPool : public ObjectPool
{
   friend class Singleton< DipIpBufferPool >;
public:
   //> The size of DipIpBuffer blocks.
   //
   static const size_t BlockSize;
private:
   //  Private because this singleton is not subclassed.
   //
   DipIpBufferPool();

   //  Private because this singleton is not subclassed.
   //
   ~DipIpBufferPool();
};
}
#endif
