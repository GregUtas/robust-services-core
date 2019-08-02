//==============================================================================
//
//  MsgHeader.h
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
#ifndef MSGHEADER_H_INCLUDED
#define MSGHEADER_H_INCLUDED

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include "LocalAddress.h"
#include "Message.h"
#include "SbTypes.h"
#include "SysSocket.h"

//------------------------------------------------------------------------------

namespace SessionBase
{
//  Header for each SessionBase message.
//
struct MsgHeader
{
   LocalAddress      txAddr;         // source address
   LocalAddress      rxAddr;         // destination address
   Message::Priority priority : 2;   // message's priority
   bool              initial  : 1;   // true for initial message
   bool              final    : 1;   // true for final message
   bool              join     : 1;   // true to create PSM and join root SSM
   bool              self     : 1;   // true for message to self
   bool              injected : 1;   // true if sent by InjectCommand
   bool              kill     : 1;   // true to kill context on arrival
   uint8_t           spare    : 6;   // reserved for future use
   Message::Route    route    : 2;   // the route that the message took
   ProtocolId        protocol : 16;  // message's protocol
   SignalId          signal   : 16;  // message's signal
   uint16_t          length   : 16;  // total bytes in all parameters

   //  Initializes all fields.
   //
   MsgHeader();

   //  Displays member variables.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;
};

//  The maximum size of the payload portion of a SessionBase message.
//
constexpr size_t MaxSbMsgSize =
   NetworkBase::SysSocket::MaxMsgSize - sizeof(MsgHeader);
}
#endif
