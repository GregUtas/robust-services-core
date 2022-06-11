//==============================================================================
//
//  MsgHeader.cpp
//
//  Copyright (C) 2013-2022  Greg Utas
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
#include "MsgHeader.h"
#include <ios>
#include <ostream>
#include "Debug.h"
#include "Formatters.h"
#include "Protocol.h"
#include "ProtocolRegistry.h"
#include "Registry.h"
#include "Singleton.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
MsgHeader::MsgHeader() :
   priority(INGRESS),
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
   Debug::ft("MsgHeader.ctor");
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
   stream << "  self=" << self;
   stream << CRLF << prefix;
   stream << "  injected=" << injected;
   stream << "  kill=" << kill;
   stream << "  route=" << int(route);
   stream << "  length=" << length;
   stream << "  spare=" << strHex(spare);
   stream << std::boolalpha;

   stream << CRLF << prefix;
   stream << "protocol=" << protocol;
   auto pro = Singleton<ProtocolRegistry>::Instance()->Protocols().At(protocol);
   stream << " (" << strClass(pro, false) << ")  signal=" << signal;
   if(pro != nullptr)
      stream << " (" << strClass(pro->GetSignal(signal), false) << ')';
   stream << CRLF;
}
}
