//==============================================================================
//
//  TlvProtocol.cpp
//
//  Copyright (C) 2013-2025  Greg Utas
//
//  This file is part of the Robust Services Core (RSC).
//
//  RSC is free software: you can redistribute it and/or modify it under the
//  terms of the Lesser GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  RSC is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the Lesser GNU General Public License
//  along with RSC.  If not, see <http://www.gnu.org/licenses/>.
//
#include "TlvProtocol.h"
#include <cstddef>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "MsgHeader.h"
#include "Parameter.h"
#include "SbIpBuffer.h"
#include "SysTypes.h"
#include "TlvMessage.h"
#include "TlvParameter.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
TlvProtocol::TlvProtocol(Id prid, Id base) : Protocol(prid, base)
{
   Debug::ft("TlvProtocol.ctor");
}

//------------------------------------------------------------------------------

TlvProtocol::~TlvProtocol()
{
   Debug::ftnt("TlvProtocol.dtor");
}

//------------------------------------------------------------------------------

void TlvProtocol::DisplayMsg(ostream& stream,
   const string& prefix, const SbIpBuffer& buff) const
{
   auto lead = prefix + spaces(2);
   byte_t* bytes;
   auto hdrsize = buff.HeaderSize();
   auto bytecount = buff.Payload(bytes);

   for(size_t index = 0; index < bytecount; NO_OP)
   {
      auto pptr = reinterpret_cast<TlvParm*>(&bytes[index]);
      auto parm = Protocol::GetParameter(pptr->header.pid);

      index += sizeof(TlvParmHeader);

      if(parm != nullptr)
         stream << prefix << strClass(parm, false);
      else if(pptr->header.pid == NIL_ID)
         stream << prefix << "Deleted parameter";
      else
         stream << prefix << "Unknown parameter";

      stream << " (offset=" << hdrsize + index - sizeof(TlvParmHeader);
      stream << " pid=" << pptr->header.pid;
      stream << " plen=" << pptr->header.plen << ')' << CRLF;

      if(parm != nullptr)
      {
         parm->DisplayMsg(stream, lead, &bytes[index], pptr->header.plen);
      }

      index += TlvMessage::Pad(pptr->header.plen);
   }
}

//------------------------------------------------------------------------------

SignalId TlvProtocol::ExtractSignal(const SbIpBuffer& buff) const
{
   Debug::ft("TlvProtocol.ExtractSignal");

   return buff.Header()->signal;
}

//------------------------------------------------------------------------------

void TlvProtocol::Patch(sel_t selector, void* arguments)
{
   Protocol::Patch(selector, arguments);
}
}
