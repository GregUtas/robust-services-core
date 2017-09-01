//==============================================================================
//
//  TlvProtocol.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
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

using std::ostream;
using std::string;
using namespace NodeBase;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name TlvProtocol_ctor = "TlvProtocol.ctor";

TlvProtocol::TlvProtocol(Id prid, Id base) : Protocol(prid, base)
{
   Debug::ft(TlvProtocol_ctor);
}

//------------------------------------------------------------------------------

fn_name TlvProtocol_dtor = "TlvProtocol.dtor";

TlvProtocol::~TlvProtocol()
{
   Debug::ft(TlvProtocol_dtor);
}

//------------------------------------------------------------------------------

void TlvProtocol::DisplayMsg(ostream& stream,
   const string& prefix, const SbIpBuffer& buff) const
{
   auto lead = prefix + spaces(2);
   byte_t* bytes;
   auto bytecount = buff.Payload(bytes);

   for(size_t index = 0; index < bytecount; NO_OP)
   {
      auto pptr = reinterpret_cast< TlvParmPtr >(&bytes[index]);
      auto parm = Protocol::GetParameter(pptr->header.pid);

      index += sizeof(TlvParmHeader);

      if(parm != nullptr)
      {
         stream << prefix << strClass(parm, false) << CRLF;
         parm->DisplayMsg(stream, lead, &bytes[index], pptr->header.plen);
      }
      else
      {
         if(pptr->header.pid == NIL_ID)
            stream << prefix << "Deleted parameter: pid=";
         else
            stream << prefix << "Unknown parameter: pid=";
         stream << pptr->header.pid;
         stream << ", length=" << pptr->header.plen << CRLF;
      }

      index += TlvMessage::Pad(pptr->header.plen);
   }
}

//------------------------------------------------------------------------------

fn_name TlvProtocol_ExtractSignal = "TlvProtocol.ExtractSignal";

SignalId TlvProtocol::ExtractSignal(const SbIpBuffer& buff) const
{
   Debug::ft(TlvProtocol_ExtractSignal);

   return buff.Header()->signal;
}

//------------------------------------------------------------------------------

void TlvProtocol::Patch(sel_t selector, void* arguments)
{
   Protocol::Patch(selector, arguments);
}
}
