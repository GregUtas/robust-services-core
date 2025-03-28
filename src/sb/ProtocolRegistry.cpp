//==============================================================================
//
//  ProtocolRegistry.cpp
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
#include "ProtocolRegistry.h"
#include <iomanip>
#include <ostream>
#include <string>
#include "Debug.h"
#include "Formatters.h"
#include "Protocol.h"
#include "SysTypes.h"

using namespace NodeBase;
using std::ostream;
using std::setw;
using std::string;

//------------------------------------------------------------------------------

namespace SessionBase
{
ProtocolRegistry::ProtocolRegistry()
{
   Debug::ft("ProtocolRegistry.ctor");

   protocols_.Init(Protocol::MaxId, Protocol::CellDiff(), MemImmutable);
}

//------------------------------------------------------------------------------

fn_name ProtocolRegistry_dtor = "ProtocolRegistry.dtor";

ProtocolRegistry::~ProtocolRegistry()
{
   Debug::ftnt(ProtocolRegistry_dtor);

   Debug::SwLog(ProtocolRegistry_dtor, UnexpectedInvocation, 0);
}

//------------------------------------------------------------------------------

bool ProtocolRegistry::BindProtocol(Protocol& protocol)
{
   Debug::ft("ProtocolRegistry.BindProtocol");

   return protocols_.Insert(protocol);
}

//------------------------------------------------------------------------------

void ProtocolRegistry::Display(ostream& stream,
   const string& prefix, const Flags& options) const
{
   Immutable::Display(stream, prefix, options);

   stream << prefix << "protocols [ProtocolId]" << CRLF;
   protocols_.Display(stream, prefix + spaces(2), options);
}

//------------------------------------------------------------------------------

void ProtocolRegistry::Patch(sel_t selector, void* arguments)
{
   Immutable::Patch(selector, arguments);
}

//------------------------------------------------------------------------------

fixed_string ProtocolHeader = " Id  Base  Signals  Parameters  Name";
//                            |  3     6        9          12..<name>

size_t ProtocolRegistry::Summarize(ostream& stream, uint32_t selector) const
{
   stream << ProtocolHeader << CRLF;

   for(auto p = protocols_.First(); p != nullptr; protocols_.Next(p))
   {
      stream << setw(3) << p->Prid();
      stream << setw(6) << p->GetBase();
      stream << setw(9) << p->Signals().Size();
      stream << setw(12) << p->Parameters().Size();
      stream << spaces(2) << strClass(p) << CRLF;
   }

   return protocols_.Size();
}

//------------------------------------------------------------------------------

void ProtocolRegistry::UnbindProtocol(Protocol& protocol)
{
   Debug::ftnt("ProtocolRegistry.UnbindProtocol");

   protocols_.Erase(protocol);
}
}
