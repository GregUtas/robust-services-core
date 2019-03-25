//==============================================================================
//
//  SbExtInputHandler.h
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
#ifndef SBEXTINPUTHANDLER_H_INCLUDED
#define SBEXTINPUTHANDLER_H_INCLUDED

#include "SbInputHandler.h"
#include "NwTypes.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Input handler for external protocols received by SessionBase applications.
//  Subclasses implement ReceiveBuff to construct a MsgHeader for the incoming
//  message and then invoke SbExtInputHandler::ReceiveBuff (inherited from our
//  base class) to queue the message for processing.  Message unbundling (e.g.
//  of messages arriving over TCP) is not supported, as unbundling procedures
//  are protocol specific.  Where a bundled message ends is usually determined
//  either by a header that contains a message length, or by an end-of-message
//  marker (e.g. <CR><LF> in a text-oriented protocol).
//
class SbExtInputHandler : public SbInputHandler
{
public:
   //  Virtual to allow subclassing.
   //
   virtual ~SbExtInputHandler();

   //  Overridden for patching.
   //
   void Patch(sel_t selector, void* arguments) override;
protected:
   //  Registers the input handler against PORT.  Protected because this
   //  class is virtual.
   //
   explicit SbExtInputHandler(NetworkBase::IpPort* port);

   //  Overridden to allocate an SbIpBuffer for an incoming external message
   //  whose MsgHeader must be built.
   //
   NetworkBase::IpBuffer* AllocBuff
      (const NodeBase::byte_t* source, size_t size, NodeBase::byte_t*& dest,
      size_t& rcvd, NetworkBase::SysTcpSocket* socket) const override;
};
}
#endif
