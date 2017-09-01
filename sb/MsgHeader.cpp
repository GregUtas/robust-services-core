//==============================================================================
//
//  MsgHeader.cpp
//
//  Copyright (C) 2012-2017 Greg Utas.  All rights reserved.
//
#include "MsgHeader.h"
#include <ios>
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "Protocol.h"
#include "ProtocolRegistry.h"
#include "Signal.h"
#include "Singleton.h"
#include "SysTypes.h"

using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
fn_name MsgHeader_ctor = "MsgHeader.ctor";

MsgHeader::MsgHeader() :
   txAddr(NilLocalAddress),
   rxAddr(NilLocalAddress),
   priority(Message::Ingress),
   initial(false),
   final(false),
   join(false),
   self(false),
   injected(false),
   kill(false),
   spare(0),
   route(Message::External),
   protocol(NIL_ID),
   signal(NIL_ID),
   length(0)
{
   Debug::ft(MsgHeader_ctor);
}

//------------------------------------------------------------------------------

void MsgHeader::Display(ostream& stream, const string& prefix) const
{
   stream << prefix << "txAddr : " << txAddr.to_str() << CRLF;
   stream << prefix << "rxAddr : " << rxAddr.to_str() << CRLF;

   stream << prefix;
   stream << std::noboolalpha;
   stream << "priority=" << int(priority);
   stream << "  initial=" << initial;
   stream << "  final=" << final;
   stream << "  join=" << join;
   stream << "  self=" << self << CRLF;
   stream << "  injected=" << injected;
   stream << "  kill=" << kill;
   stream << "  route=" << int(route);
   stream << "  length=" << length;
   stream << "  spare=" << strHex(spare);
   stream << CRLF;
   stream << std::boolalpha;

   stream << prefix;
   stream << "protocol=" << protocol;
   auto pro = Singleton< ProtocolRegistry >::Instance()->GetProtocol(protocol);
   stream << " (" << strClass(pro, false) << ")  signal=" << signal;
   if(pro != nullptr)
      stream << " (" << strClass(pro->GetSignal(signal), false) << ')';
   stream << CRLF;
}
}
