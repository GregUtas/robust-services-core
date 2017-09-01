//==============================================================================
//
//  MsgHeader.h
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#ifndef MSGHEADER_H_INCLUDED
#define MSGHEADER_H_INCLUDED

#include <cstdint>
#include <iosfwd>
#include <string>
#include "LocalAddress.h"
#include "Message.h"
#include "NbTypes.h"
#include "SbTypes.h"
#include "SysSocket.h"

using namespace NodeBase;

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
   MsgSize           length   : 16;  // total bytes in all parameters

   //  The maximum size of the payload portion of a SessionBase message.
   //  The magic "32" must be sizeof(MsgHeader) or greater.
   //
   static const MsgSize MaxMsgSize = (SysSocket::MaxMsgSize - 32);

   //  Initializes all fields.
   //
   MsgHeader();

   //  Displays member variables.
   //
   void Display(std::ostream& stream, const std::string& prefix) const;
};
}
#endif
